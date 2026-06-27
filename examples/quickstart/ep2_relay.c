/* Quickstart endpoint 2 (C / c-xml-expat): a relay.
 *
 * Reads a <message> from stdin (produced by the Python endpoint upstream),
 * unmarshals it with the xsdb-generated C streaming bindings, appends this
 * endpoint's <hop>, and writes the whole message to stdout for the next
 * endpoint. This is the C counterpart of ep5 (C++ / cpp-xml-expat): both stream
 * via expat, but the C target uses function-pointer callbacks and structs.
 *
 * The streaming parser reports each element through an unmarshal_<name> callback
 * at its end tag (children before parent) and frees its struct afterwards, so we
 * copy out the strings we want to keep, then re-marshal in schema order. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xml_message.h"

/* --- captured message (own copies; the parser frees its structs post-call) --- */
static char*   g_id;
static char*   g_kind;
static char*   g_subject;
static uint8_t g_priority;

typedef struct { char* endpoint; char* lang; int32_t seq; } HopRec;
static HopRec* g_hops;
static size_t  g_nhops;

/* endpoint/lang close (and fire) before their enclosing hop */
static char* g_pendEndpoint;
static char* g_pendLang;

static char* dupOrNull(const char* s) {
	if (!s) return NULL;
	size_t n = strlen(s) + 1;
	char* d = (char*)malloc(n);
	memcpy(d, s, n);
	return d;
}

/* --- unmarshal callbacks (post-order: children before parent) --- */
static void cap_subject(struct xml_marshaller* m, const xml_subject* o, uint32_t p) {
	(void)m; (void)p; free(g_subject); g_subject = dupOrNull(o->pContent_);
}
static void cap_priority(struct xml_marshaller* m, const xml_priority* o, uint32_t p) {
	(void)m; (void)p; g_priority = o->Content_;
}
static void cap_header(struct xml_marshaller* m, const xml_header* o, uint32_t p) {
	(void)m; (void)p;
	free(g_id);   g_id   = dupOrNull(o->pid_);
	free(g_kind); g_kind = dupOrNull(o->pkind_);
}
static void cap_endpoint(struct xml_marshaller* m, const xml_endpoint* o, uint32_t p) {
	(void)m; (void)p; free(g_pendEndpoint); g_pendEndpoint = dupOrNull(o->pContent_);
}
static void cap_lang(struct xml_marshaller* m, const xml_lang* o, uint32_t p) {
	(void)m; (void)p; free(g_pendLang); g_pendLang = dupOrNull(o->pContent_);
}
static void cap_hop(struct xml_marshaller* m, const xml_hop* o, uint32_t p) {
	(void)m; (void)p;
	g_hops = (HopRec*)realloc(g_hops, (g_nhops + 1) * sizeof(*g_hops));
	g_hops[g_nhops].endpoint = g_pendEndpoint;
	g_hops[g_nhops].lang     = g_pendLang;
	g_hops[g_nhops].seq      = o->seq_;
	g_pendEndpoint = g_pendLang = NULL;
	g_nhops++;
}
static void cap_message(struct xml_marshaller* m, const xml_message* o, uint32_t p) {
	(void)m; (void)o; (void)p;
}

static void* reallocCb(struct xml_marshaller* m, void* p, size_t size) {
	(void)m; return realloc(p, size);
}

/* slurp all of stdin into a malloc'd buffer */
static char* readAll(size_t* outLen) {
	size_t cap = 4096, len = 0;
	char* buf = (char*)malloc(cap);
	for (;;) {
		if (len == cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
		size_t n = fread(buf + len, 1, cap - len, stdin);
		len += n;
		if (n == 0) break;
	}
	*outLen = len;
	return buf;
}

int main(void) {
	size_t inLen;
	char* inData = readAll(&inLen);

	xml_marshaller m = {
		.realloc            = reallocCb,
		.unmarshal_subject  = cap_subject,
		.unmarshal_priority = cap_priority,
		.unmarshal_header   = cap_header,
		.unmarshal_endpoint = cap_endpoint,
		.unmarshal_lang     = cap_lang,
		.unmarshal_hop      = cap_hop,
		.unmarshal_message  = cap_message,
	};

	xml_buffer in = { (uint8_t*)inData, (uint32_t)inLen, (uint32_t)inLen };
	xml_unmarshal(&m, &in, 1);

	/* re-marshal in schema order (xml_marshal emits the <?xml?> prologue),
	   appending this endpoint's hop */
	xml_marshal_message_xsd root = xml_marshal(&m);
	xml_message msg = {0};
	xml_marshal_message mm = root.marshal_message(&m, &msg);

	xml_header hdr = {0}; hdr.pid_ = g_id; hdr.pkind_ = g_kind;
	xml_marshal_header mh = mm.marshal_header(&m, &hdr);
	xml_subject subj = {0}; subj.pContent_ = g_subject;
	mh.marshal_subject(&m, &subj);
	xml_priority pri = {0}; pri.Content_ = g_priority;
	mh.marshal_priority(&m, &pri);

	for (size_t i = 0; i < g_nhops; i++) {
		xml_hop hop = {0}; hop.seq_ = g_hops[i].seq;
		xml_marshal_hop mhop = mm.marshal_hop(&m, &hop);
		xml_endpoint ep = {0}; ep.pContent_ = g_hops[i].endpoint;
		mhop.marshal_endpoint(&m, &ep);
		xml_lang lg = {0}; lg.pContent_ = g_hops[i].lang;
		mhop.marshal_lang(&m, &lg);
	}

	xml_hop newHop = {0}; newHop.seq_ = (int32_t)(g_nhops + 1);
	xml_marshal_hop mnh = mm.marshal_hop(&m, &newHop);
	xml_endpoint nep = {0}; nep.pContent_ = (char*)"ep2";
	mnh.marshal_endpoint(&m, &nep);
	xml_lang nlg = {0}; nlg.pContent_ = (char*)"c";
	mnh.marshal_lang(&m, &nlg);

	xml_buffer out = xml_marshal_flush(&m, 1);
	fwrite(out.pBuf_, 1, out.used_, stdout);
	fputc('\n', stdout);

	/* tidy up our own allocations (the process exits next regardless) */
	free(out.pBuf_);
	free(inData);
	free(g_id); free(g_kind); free(g_subject);
	for (size_t i = 0; i < g_nhops; i++) { free(g_hops[i].endpoint); free(g_hops[i].lang); }
	free(g_hops); free(g_pendEndpoint); free(g_pendLang);
	return 0;
}
