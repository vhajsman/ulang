#include "compiler.hpp"
#include <string>

namespace ULang {
    std::string h_severityStr(CompilerSyntaxException::Severity severity) {
        switch(severity) {
            case CompilerSyntaxException::Severity::Error:
                return "Error  ";
            case CompilerSyntaxException::Severity::Warning:
                return "Warning";
            case CompilerSyntaxException::Severity::Note:
                return "Note   ";
        }
    }

    std::string h_severityStr_json(CompilerSyntaxException::Severity severity) {
        switch(severity) {
            case CompilerSyntaxException::Severity::Error:
                return "error";
            case CompilerSyntaxException::Severity::Warning:
                return "warning";
            case CompilerSyntaxException::Severity::Note:
                return "note";
        }
    }

    std::string h_severityColor(CompilerSyntaxException::Severity severity) {
        switch(severity) {
            case CompilerSyntaxException::Severity::Error:
                return "\033[31;1;4m";
            case CompilerSyntaxException::Severity::Warning:
                return "\033[33;1;4m";
            case CompilerSyntaxException::Severity::Note:
                return "\033[34;1;4m";
        }
    }

    CompilerSyntaxException::CompilerSyntaxException(Severity severity, const std::string& m, SourceLocation loc, unsigned int errnum)
    : severity(severity), loc(loc), msg(m), errnum(errnum) {}

    //const char* CompilerSyntaxException::what() const noexcept override {
    //    return this->msg.c_str();
    //}

    CompilerSyntaxException::Severity CompilerSyntaxException::getSeverity() const {
        return this->severity;
    }

    SourceLocation CompilerSyntaxException::getLocation() const {
        return this->loc;
    }

    std::string CompilerSyntaxException::fmt(bool en_color, std::string line) const {
        /*
            Error   | src/test1.ul@17:0 unknown type 'abc' (101) :
                      abc myVar = 1234567890;
        */
        std::string out;
        
        if(en_color)
            out.append(h_severityColor(this->severity));
        out.append(h_severityStr(this->severity));
        if(en_color)
            out.append("\033[0m");

        out.append(" | ");
        out.append(this->loc.loc_file + "@" + std::to_string(this->loc.loc_line) + ":" + std::to_string(this->loc.loc_col));
        out.append(" " + this->msg + " ");

        if(this->errnum != 0)
            out.append("(" + std::to_string(this->errnum) + ") ");

        if(line.size() != 0) {
            out.append(":\n");
            out.append("          ");
            out.append(line);
            
            size_t col = static_cast<size_t>(this->loc.loc_col);
            if(col > line.size()) 
                col = line.size();

            out.append(std::string(col + 10, ' ') + "^^^");
        }

        return out;
    }

    std::string CompilerSyntaxException::fmt_json() const {
        /*
            {
                "file": "src/test1.ul",
                "line": 17,
                "column": 0,
                "severity": "error",
                "message": "unknown type 'abc'",
                "errno": 101
            }
        */
        std::string out;

        out.append("{");

        out.append("\"file\": \"" + this->loc.loc_file + "\",");
        out.append("\"line\": " + std::to_string(this->loc.loc_line) + ",");
        out.append("\"column\": " + std::to_string(this->loc.loc_col) + ",");

        out.append("\"severity\": \"" + h_severityStr_json(this->severity) + "\",");
        out.append("\"message\":\"" + this->msg + "\",");
        out.append("\"errno\": " + std::to_string(this->errnum) + "");

        out.append("}");

        return out;
    }
};