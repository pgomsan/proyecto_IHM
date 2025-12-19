#include "helpdialog.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QTextBrowser>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Ayuda"));
    setModal(true);
    resize(620, 560);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    m_browser = new QTextBrowser(this);
    m_browser->setOpenExternalLinks(true);
    m_browser->setReadOnly(true);

    QFile file(QStringLiteral(":/help/ayuda.html"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_browser->setHtml(QString::fromUtf8(file.readAll()));
    } else {
        m_browser->setPlainText(tr("No se pudo cargar la ayuda."));
    }

    layout->addWidget(m_browser, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}
