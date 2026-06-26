// Quickstart endpoint 3 (Java / java-xml-stax): a relay.
//
// Reads a <message> from stdin (produced by the C++ endpoint upstream),
// unmarshals it with the xsdb-generated bindings, appends this endpoint's
// <hop>, and writes the whole message to stdout for the next endpoint. The
// generated classes (Document, message, hop, endpoint, lang, Element) all live
// in the `interop` package, set via `xsdb … java-xml-stax … -package interop`.
package interop;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

public class Ep3Relay {
	public static void main(String[] args) throws Exception {
		ByteArrayOutputStream buffer = new ByteArrayOutputStream();
		byte[] chunk = new byte[8192];
		for (int n; (n = System.in.read(chunk)) != -1; ) {
			buffer.write(chunk, 0, n);
		}
		String in = new String(buffer.toByteArray(), StandardCharsets.UTF_8);

		Document doc = new Document();
		message msg = (message) doc.unmarshal(in);

		int hops = 0;
		for (Element child : msg.children()) {
			if (child instanceof hop) {
				hops++;
			}
		}

		endpoint endpoint = new endpoint();
		endpoint.set_text("ep3");
		lang lang = new lang();
		lang.set_text("java");
		hop hop = new hop();
		hop.set_seq(hops + 1);
		hop.children().add(endpoint);
		hop.children().add(lang);
		msg.children().add(hop);

		System.out.print(doc.marshal(msg));
	}
}
