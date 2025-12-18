#include "converter.h"
#include <algorithm>
#include <cctype>

Format parseFormat(const std::string& formatStr) {
    std::string lower;
    lower.reserve(formatStr.size());
    std::transform(formatStr.begin(), formatStr.end(), std::back_inserter(lower),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    if (lower == "json") {
        return Format::JSON;
    } else if (lower == "toml") {
        return Format::TOML;
    } else if (lower == "xml") {
        return Format::XML;
    } else {
        return Format::UNKNOWN;
    }
}

std::shared_ptr<ASTNode> parseFromFormat(Format format, const std::string& input) {
    switch (format) {
        case Format::JSON:
            return parseWithFormat<Format::JSON>(input);
        case Format::TOML:
            return parseWithFormat<Format::TOML>(input);
        case Format::XML:
            return parseWithFormat<Format::XML>(input);
        default:
            throw std::runtime_error("Unsupported input format");
    }
}

std::string serializeToFormat(Format format, const std::shared_ptr<ASTNode>& node) {
    switch (format) {
        case Format::JSON:
            return serializeWithFormat<Format::JSON>(node);
        case Format::TOML:
            return serializeWithFormat<Format::TOML>(node);
        case Format::XML:
            return serializeWithFormat<Format::XML>(node);
        default:
            throw std::runtime_error("Unsupported output format");
    }
}

namespace {
int formatToIndex(Format format) {
    switch (format) {
        case Format::JSON: return 0;
        case Format::TOML: return 1;
        case Format::XML:  return 2;
        default:           return -1;
    }
}

template<Format InputFmt, Format OutputFmt>
std::string convertImpl(const std::string& input) {
    FormatConverter<InputFmt, OutputFmt> converter;
    return converter.convert(input);
}
}

std::string convertDocument(Format inputFormat, Format outputFormat, const std::string& input) {
    const int inputIndex = formatToIndex(inputFormat);
    const int outputIndex = formatToIndex(outputFormat);
    
    if (inputIndex < 0) {
        throw std::runtime_error("Unsupported input format");
    }
    if (outputIndex < 0) {
        throw std::runtime_error("Unsupported output format");
    }
    
    using ConvertFn = std::string(*)(const std::string&);
    static const ConvertFn dispatch[3][3] = {
        {
            convertImpl<Format::JSON, Format::JSON>,
            convertImpl<Format::JSON, Format::TOML>,
            convertImpl<Format::JSON, Format::XML>
        },
        {
            convertImpl<Format::TOML, Format::JSON>,
            convertImpl<Format::TOML, Format::TOML>,
            convertImpl<Format::TOML, Format::XML>
        },
        {
            convertImpl<Format::XML, Format::JSON>,
            convertImpl<Format::XML, Format::TOML>,
            convertImpl<Format::XML, Format::XML>
        }
    };
    
    return dispatch[inputIndex][outputIndex](input);
}

