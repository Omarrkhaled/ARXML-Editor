// arxml_validator.cpp
//
// Implements ArxmlValidator. Validation is performed by running the
// `xmllint` command-line tool via QProcess. The model's document is
// written to a temporary ARXML file, then validated against the given
// schema. The command's stderr output is captured and returned to
// indicate errors.

#include "arxml_validator.hpp"
#include "arxml_model.hpp"

#include <QDomDocument>
#include <QTemporaryFile>
#include <QProcess>
#include <QDir>
#include <QTextStream>

QString ArxmlValidator::validate(const ArxmlModel &model, const QString &schemaFile) const
{
    // Write the current document to a temporary file
    QTemporaryFile tmpFile(QDir::tempPath() + QLatin1String("/arxml_validateXXXXXX.arxml"));
    if (!tmpFile.open()) {
        return QStringLiteral("Failed to create temporary file for validation.");
    }
    QTextStream out(&tmpFile);
    model.document().save(out, 4);
    out.flush();
    tmpFile.close();

    // Prepare arguments for xmllint
    QStringList args;
    args << "--noout" << "--schema" << schemaFile << tmpFile.fileName();

    QProcess process;
    process.start("xmllint", args);
    if (!process.waitForFinished(10000)) {
        return QStringLiteral("xmllint did not finish within the timeout.");
    }

    const QByteArray stderrOutput = process.readAllStandardError();
    // A zero exit code indicates success. If non-zero, return stderr or a
    // generic error message.
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        return QString();
    }
    if (!stderrOutput.isEmpty()) {
        return QString::fromUtf8(stderrOutput);
    }
    return QStringLiteral("Unknown validation error.");
}