#include "AppImageManager/SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QVBoxLayout>

namespace appimagemanager {

SettingsDialog::SettingsDialog(const Preferences &preferences, QWidget *parent)
    : QDialog(parent)
    , m_initialPreferences(preferences)
    , m_moveToStorageCheck(nullptr)
    , m_confirmRemovalCheck(nullptr)
    , m_listViewRadio(nullptr)
    , m_gridViewRadio(nullptr)
    , m_languageCombo(nullptr)
    , m_buttonBox(nullptr)
    , m_generalGroup(nullptr)
    , m_viewGroup(nullptr)
    , m_languageGroup(nullptr)
{
    buildUi();
    applyInitialState(preferences);
    retranslateUi();

    setModal(true);
    setMinimumWidth(420);
}

void SettingsDialog::buildUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    m_generalGroup = new QGroupBox(this);
    auto *generalLayout = new QVBoxLayout(m_generalGroup);
    generalLayout->setContentsMargins(12, 12, 12, 12);
    generalLayout->setSpacing(8);

    m_moveToStorageCheck = new QCheckBox(m_generalGroup);
    generalLayout->addWidget(m_moveToStorageCheck);

    m_confirmRemovalCheck = new QCheckBox(m_generalGroup);
    generalLayout->addWidget(m_confirmRemovalCheck);

    m_viewGroup = new QGroupBox(this);
    auto *viewLayout = new QHBoxLayout(m_viewGroup);
    viewLayout->setContentsMargins(12, 12, 12, 12);
    viewLayout->setSpacing(12);

    m_listViewRadio = new QRadioButton(m_viewGroup);
    m_gridViewRadio = new QRadioButton(m_viewGroup);

    viewLayout->addWidget(m_listViewRadio);
    viewLayout->addWidget(m_gridViewRadio);
    viewLayout->addStretch(1);

    m_languageGroup = new QGroupBox(this);
    auto *languageLayout = new QVBoxLayout(m_languageGroup);
    languageLayout->setContentsMargins(12, 12, 12, 12);
    languageLayout->setSpacing(8);

    m_languageCombo = new QComboBox(m_languageGroup);
    m_languageCombo->addItem(QString(), static_cast<int>(LanguageOption::System));
    m_languageCombo->addItem(QString(), static_cast<int>(LanguageOption::English));
    m_languageCombo->addItem(QString(), static_cast<int>(LanguageOption::ChineseSimplified));
    languageLayout->addWidget(m_languageCombo);

    mainLayout->addWidget(m_generalGroup);
    mainLayout->addWidget(m_viewGroup);
    mainLayout->addWidget(m_languageGroup);
    mainLayout->addStretch(1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::applyInitialState(const Preferences &preferences)
{
    m_moveToStorageCheck->setChecked(preferences.moveToStorageOnAdd);
    m_confirmRemovalCheck->setChecked(preferences.confirmRemoval);

    if (preferences.viewMode == ViewMode::Grid) {
        m_gridViewRadio->setChecked(true);
    } else {
        m_listViewRadio->setChecked(true);
    }

    const int index = m_languageCombo->findData(static_cast<int>(preferences.language));
    if (index >= 0) {
        m_languageCombo->setCurrentIndex(index);
    }
}

Preferences SettingsDialog::preferences() const
{
    Preferences prefs = m_initialPreferences;
    prefs.moveToStorageOnAdd = m_moveToStorageCheck->isChecked();
    prefs.confirmRemoval = m_confirmRemovalCheck->isChecked();
    prefs.viewMode = m_gridViewRadio->isChecked() ? ViewMode::Grid : ViewMode::List;
    prefs.language = static_cast<LanguageOption>(m_languageCombo->currentData().toInt());
    return prefs;
}

void SettingsDialog::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void SettingsDialog::retranslateUi()
{
    setWindowTitle(tr("Preferences"));

    if (m_generalGroup) {
        m_generalGroup->setTitle(tr("General"));
    }
    if (m_moveToStorageCheck) {
        m_moveToStorageCheck->setText(tr("Move AppImages into managed storage"));
    }
    if (m_confirmRemovalCheck) {
        m_confirmRemovalCheck->setText(tr("Ask for confirmation before removing"));
    }

    if (m_viewGroup) {
        m_viewGroup->setTitle(tr("Layout"));
    }
    if (m_listViewRadio) {
        m_listViewRadio->setText(tr("List view"));
    }
    if (m_gridViewRadio) {
        m_gridViewRadio->setText(tr("Grid view"));
    }

    if (m_languageGroup) {
        m_languageGroup->setTitle(tr("Language"));
    }
    if (m_languageCombo) {
        m_languageCombo->setItemText(0, tr("System default"));
        m_languageCombo->setItemText(1, tr("English"));
        m_languageCombo->setItemText(2, tr("Chinese (Simplified)"));
    }
}

} // namespace appimagemanager
