#include "RecSpec.hpp"

#include <cstring>
#include <sstream>

namespace sb {

static bool parseField(const std::string& tok, RecField& out, std::string& err) {
    size_t colon = tok.find(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= tok.size()) {
        err = "bad field '" + tok + "' (want role:type)";
        return false;
    }
    out.role = tok.substr(0, colon);
    std::string t = tok.substr(colon + 1);
    out.bigEndian = false;
    if (!t.empty() && t.back() == '>') {
        out.bigEndian = true;
        t.pop_back();
    }
    if (t == "B" || t == "b") out.size = 1;
    else if (t == "H" || t == "h") out.size = 2;
    else if (t == "I" || t == "i") out.size = 4;
    else {
        err = "bad type '" + t + "' (B b H h I i)";
        return false;
    }
    out.type = t[0];
    return true;
}

RecSpec parseRecSpec(const std::string& text) {
    RecSpec s;
    std::istringstream in(text);
    std::string tok;
    size_t off = 0;
    while (in >> tok) {
        RecField f;
        if (!parseField(tok, f, s.error)) return s;
        f.offset = off;
        off += f.size;
        s.fields.push_back(std::move(f));
    }
    s.size = (int)off;
    s.valid = !s.fields.empty();
    if (!s.valid) s.error = "empty spec";
    return s;
}

RecValues decodeRecord(const RecSpec& spec, const uint8_t* rec) {
    RecValues out;
    for (const auto& f : spec.fields) {
        const uint8_t* p = rec + f.offset;
        uint32_t v = 0;
        if (f.bigEndian) {
            for (int i = 0; i < f.size; ++i) v = (v << 8) | p[i];
        } else {
            for (int i = f.size - 1; i >= 0; --i) v = (v << 8) | p[i];
        }
        out.v[f.role] = v;
    }
    return out;
}

const std::map<int, std::string>& defaultRecSpecs() {
    // the verified layouts from docs/qmd-format.md
    static const std::map<int, std::string> defs = {
        {0, "v0:H v1:H v2:H v3:H n0a:b n0b:b n0c:b n1a:b n1b:b n1c:b n2a:b n2b:b n2c:b n3a:b n3b:b n3c:b"
             " c0:I c1:I c2:I c3:I"},
        {1, "v0:H v1:H v2:H n0a:b n0b:b n0c:b n1a:b n1b:b n1c:b n2a:b n2b:b n2c:b x:B c0:I c1:I c2:I"},
        {2, "v0:H v1:H v2:H v3:H n0a:b n0b:b n0c:b n1a:b n1b:b n1c:b n2a:b n2b:b n2c:b n3a:b n3b:b n3c:b rgb:I"},
        {3, "v0:H v1:H v2:H n0a:b n0b:b n0c:b n1a:b n1b:b n1c:b n2a:b n2b:b n2c:b rgb:B x:B x:B x:B"},
        {4, "v0:H v1:H v2:H v3:H c0:I c1:I c2:I c3:I"},
        {5, "v0:H v1:H v2:H c0:I c1:I c2:I x:B x:B x:B x:B x:B x:B"},
        {6, "v0:H v1:H v2:H v3:H x:I x:I x:I rgb:I"},
        {8, "v0:H v1:H v2:H v3:H u2:B w2:B u3:B w3:B u0:B w0:B cba:H u1:B w1:B tp:H rgb:I n0:H n1:H n2:H n3:H"},
        {9, "v0:H v1:H v2:H u2:B w2:B u0:B w0:B cba:H u1:B w1:B tp:H rgb:I n0:H n1:H n2:H x:H"},
        {10, "v0:H v1:H v2:H v3:H u2:B w2:B u3:B w3:B u0:B w0:B cba:H u1:B w1:B tp:H rgb:I n0:H n1:H n2:H n3:H"},
        {11, "v0:H v1:H v2:H u2:B w2:B u0:B w0:B cba:H u1:B w1:B tp:H rgb:I n0:H n1:H n2:H x:H"},
    };
    return defs;
}

} // namespace sb
