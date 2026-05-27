#pragma once
#include <QString>
#include <vector>

struct ParsedParameter {
    QString varName;
    QString displayName;
    double minValue = 0.0;
    double maxValue = 100.0;
    double defaultValue = 0.0;
    int decimals = 0;
};

class ParameterParser {
public:
    static std::vector<ParsedParameter> parseCode(const QString& code);
};
