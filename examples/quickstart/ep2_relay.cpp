// Quickstart endpoint 2 (C++ / cpp-xml-expat): a relay.
//
// Reads a <message> from stdin (produced by the Python endpoint upstream),
// unmarshals it with the xsdb-generated bindings, appends this endpoint's
// <hop>, and writes the whole message to stdout for the next endpoint.
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "relay_util.hpp"

int main() try {
	std::ostringstream in;
	in << std::cin.rdbuf();
	const std::string input = in.str();

	MessageCapture cap;
	cap.unmarshal(input.data(), input.size(), true);

	const std::string doc = appendHopAndMarshal(cap, "ep2", "c++");
	std::fwrite(doc.data(), 1, doc.size(), stdout);
	std::fputc('\n', stdout);
	return 0;
} catch (const std::exception& e) {
	std::fprintf(stderr, "ep2 failed: %s\n", e.what());
	return 1;
}
