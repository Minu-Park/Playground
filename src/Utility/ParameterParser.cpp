#include "ParameterParser.h"
#include <QRegularExpression>
#include <QStringList>

std::vector<ParsedParameter> ParameterParser::parseCode(const QString& code) {
    std::vector<ParsedParameter> params;

    // Split code by lines
    QStringList lines = code.split(QRegularExpression(QString::fromUtf8("[\r\n]")), Qt::SkipEmptyParts);

    // Regex to detect "@param:" lines
    // Group 1: variable name
    // Group 2: remainder of the configuration
    QRegularExpression paramLineRegex(QString::fromUtf8("(?://\\s*)?@param:\\s*([a-zA-Z_][a-zA-Z0-9_]*)(.*)$"));

    // Sub-regexes to match key-value pairs
    QRegularExpression nameRegex(QString::fromUtf8("name\\s*=\\s*\"([^\"]*)\""));
    QRegularExpression minRegex(QString::fromUtf8("min\\s*=\\s*([0-9.-]+)"));
    QRegularExpression maxRegex(QString::fromUtf8("max\\s*=\\s*([0-9.-]+)"));
    QRegularExpression defRegex(QString::fromUtf8("default\\s*=\\s*([0-9.-]+)"));

    for (const QString& line : lines) {
        QRegularExpressionMatch lineMatch = paramLineRegex.match(line);
        if (!lineMatch.hasMatch()) {
            continue;
        }

        QString varName = lineMatch.captured(1);
        QString configStr = lineMatch.captured(2);

        ParsedParameter p;
        p.varName = varName;

        // Match displayName
        QRegularExpressionMatch nameMatch = nameRegex.match(configStr);
        if (nameMatch.hasMatch()) {
            p.displayName = nameMatch.captured(1);
        } else {
            p.displayName = varName; // fallback to variable name
        }

        // Match min
        QRegularExpressionMatch minMatch = minRegex.match(configStr);
        if (minMatch.hasMatch()) {
            p.minValue = minMatch.captured(1).toDouble();
        }

        // Match max
        QRegularExpressionMatch maxMatch = maxRegex.match(configStr);
        if (maxMatch.hasMatch()) {
            p.maxValue = maxMatch.captured(1).toDouble();
        }

        // Match default
        QRegularExpressionMatch defMatch = defRegex.match(configStr);
        if (defMatch.hasMatch()) {
            p.defaultValue = defMatch.captured(1).toDouble();
        } else {
            p.defaultValue = p.minValue;
        }

        // Auto-detect decimals
        // If min, max, or default contains a dot '.', we determine decimal digits
        int maxDecimals = 0;

        auto checkDecimals = [&](const QString& valStr) {
            int dotIdx = valStr.indexOf(QLatin1Char('.'));
            if (dotIdx >= 0) {
                int digits = valStr.length() - dotIdx - 1;
                if (digits > maxDecimals) {
                    maxDecimals = digits;
                }
            }
        };

        if (minMatch.hasMatch()) checkDecimals(minMatch.captured(1));
        if (maxMatch.hasMatch()) checkDecimals(maxMatch.captured(1));
        if (defMatch.hasMatch()) checkDecimals(defMatch.captured(1));

        p.decimals = maxDecimals;

        params.push_back(p);
    }

    return params;
}
