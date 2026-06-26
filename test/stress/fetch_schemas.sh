#!/usr/bin/env bash
# Download a curated set of large/complex public XSDs for stress testing.
# Tolerant of individual download failures (sites go down); reports what it got.
# Output: test/stress/corpus/  (gitignored — not committed; these are upstream
# documents with varied licenses, fetched on demand for discovery only).
set -u

here="$(cd "$(dirname "$0")" && pwd)"
dest="$here/corpus"
mkdir -p "$dest"

# Each entry: <filename>|<url>. Chosen to exercise the risky constructs:
#   XMLSchema.xsd  -> heavy anyAttribute; the schema-for-schemas
#   xhtml11        -> xs:redefine (XHTML modularization)
#   docbook        -> huge, deep nesting, xlink import
#   soap/wsdl      -> anyAttribute, import chains
#   UBL invoice    -> very large, identity constraints, big import graph
schemas=(
  "xmlschema.xsd|https://www.w3.org/2001/XMLSchema.xsd"
  "xml.xsd|https://www.w3.org/2001/xml.xsd"
  "xhtml1-strict.xsd|https://www.w3.org/2002/08/xhtml/xhtml1-strict.xsd"
  "xhtml11.xsd|https://www.w3.org/MarkUp/SCHEMA/xhtml11.xsd"
  "docbook.xsd|https://docbook.org/xml/5.0/xsd/docbook.xsd"
  "soap-envelope.xsd|https://schemas.xmlsoap.org/soap/envelope/"
  "wsdl.xsd|https://schemas.xmlsoap.org/wsdl/"
  "xmldsig.xsd|https://www.w3.org/TR/2002/REC-xmldsig-core-20020212/xmldsig-core-schema.xsd"
  "kml.xsd|https://developers.google.com/kml/schema/kml22gx.xsd"
)

ok=0; fail=0
for entry in "${schemas[@]}"; do
  name="${entry%%|*}"; url="${entry#*|}"
  if curl -fsSL --max-time 45 "$url" -o "$dest/$name" 2>/dev/null \
     && [ -s "$dest/$name" ]; then
    printf "  ok    %-22s %8s bytes\n" "$name" "$(wc -c <"$dest/$name")"
    ok=$((ok+1))
  else
    printf "  FAIL  %-22s %s\n" "$name" "$url"
    rm -f "$dest/$name"; fail=$((fail+1))
  fi
done
echo "fetched $ok, failed $fail  ->  $dest"
