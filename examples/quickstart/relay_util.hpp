// Shared helper for the two C++ quickstart endpoints (cpp-xml-expat bindings).
//
// The generated `Marshaller` reports each element through an `on_<name>` hook
// that fires at the element's END tag — i.e. post-order, children before their
// parent. So to relay a <message> we capture the typed children as they arrive,
// bundle each <hop>'s <endpoint>/<lang> into it, then re-emit everything in
// schema order (pre-order) with one extra <hop> appended for this endpoint.
#ifndef RELAY_UTIL_HPP
#define RELAY_UTIL_HPP

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include "xml_message.h"

class MessageCapture : public xml::Marshaller {
public:
	xml::subject  subject_;
	xml::priority priority_;
	// header has attributes -> no default ctor, so hold it by pointer and
	// copy-construct it in the callback.
	std::unique_ptr<xml::header> header_;
	struct HopRec { xml::hop hop; xml::endpoint endpoint; xml::lang lang; };
	std::vector<HopRec> hops_;

private:
	xml::endpoint pendingEndpoint_;
	xml::lang     pendingLang_;
	void on_subject (const xml::subject&  o) override { subject_  = o; }
	void on_priority(const xml::priority& o) override { priority_ = o; }
	void on_header  (const xml::header&   o) override {
		header_.reset(new xml::header(o));
	}
	void on_endpoint(const xml::endpoint& o) override { pendingEndpoint_ = o; }
	void on_lang    (const xml::lang&     o) override { pendingLang_ = o; }
	void on_hop     (const xml::hop&      o) override {
		hops_.push_back({o, pendingEndpoint_, pendingLang_});
	}
	void on_message (const xml::message&)    override {}
};

// Re-marshal the captured message in schema order, appending one <hop> for this
// endpoint; returns the XML document text. The header is reproduced unchanged.
inline std::string appendHopAndMarshal(const MessageCapture& cap,
                                       const std::string& endpointName,
                                       const std::string& langName) {
	// header is optional in the capture (unique_ptr); a well-formed message
	// always has one, so a missing header is malformed input -> throw, not deref.
	if (!cap.header_)
		throw std::runtime_error("message has no <header>");
	xml::Marshaller out;
	out.marshalBegin();
	out.marshal_message(xml::message{});
	out.marshal_header(*cap.header_);
	out.marshal_subject(cap.subject_);
	out.marshal_priority(cap.priority_);
	for (const auto& h : cap.hops_) {
		out.marshal_hop(h.hop);
		out.marshal_endpoint(h.endpoint);
		out.marshal_lang(h.lang);
	}
	// seq is an optional attribute -> unique_ptr; construct the hop with it.
	xml::hop newHop(std::unique_ptr<int32_t>(
		new int32_t(static_cast<int32_t>(cap.hops_.size() + 1))));
	xml::endpoint endpoint; endpoint.content_ = endpointName;
	xml::lang     lang;     lang.content_     = langName;
	out.marshal_hop(newHop);
	out.marshal_endpoint(endpoint);
	out.marshal_lang(lang);
	return out.flush(true);
}

#endif // RELAY_UTIL_HPP
