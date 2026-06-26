// Quickstart endpoint 5 (C++ / cpp-xml-expat): the sink + verifier.
//
// Reads the <message> from stdin (from the Python endpoint upstream), appends
// the final <hop>, writes the completed message to stdout, then asserts the
// relay worked: the header survived unchanged and exactly five hops accumulated
// with seq 1..5. Exits 0 on success (prints PASS to stderr), 1 otherwise.
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

	const std::string doc = appendHopAndMarshal(cap, "ep5", "c++");
	std::fwrite(doc.data(), 1, doc.size(), stdout);
	std::fputc('\n', stdout);

	// This endpoint's hop makes five; the four it received are seq 1..4.
	bool ok = cap.hops_.size() == 4
	          && cap.header_.kind_ == "request"
	          && cap.priority_.content_ == 3;
	for (std::size_t i = 0; i < cap.hops_.size() && ok; ++i)
		if (cap.hops_[i].hop.seq_ != static_cast<int>(i) + 1)
			ok = false;

	if (ok) {
		std::fprintf(stderr,
		             "PASS: message relayed through 5 endpoints "
		             "(python->c++->java->python->c++); header intact, "
		             "5 hops in order.\n");
		return 0;
	}
	std::fprintf(stderr, "FAIL: expected an intact header and 5 ordered hops.\n");
	return 1;
} catch (const std::exception& e) {
	std::fprintf(stderr, "ep5 failed: %s\n", e.what());
	return 1;
}
