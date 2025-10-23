#pragma once

#include <QDialog>

#include "AppImageManager/Preferences.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QRadioButton;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
QT_END_NAMESPACE

namespace appimagemanager {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const Preferences &preferences, QWidget *parent = nullptr);

    Preferences preferences() const;

protected:
    void changeEvent(QEvent *event) override;

private:
    void buildUi();
    void retranslateUi();
    void applyInitialState(const Preferences &preferences);

private:
    Preferences m_initialPreferences;
    QCheckBox *m_moveToStorageCheck;
    QCheckBox *m_confirmRemovalCheck;
    QRadioButton *m_listViewRadio;
    QRadioButton *m_gridViewRadio;
    QComboBox *m_languageCombo;
    QDialogButtonBox *m_buttonBox;
    QGroupBox *m_generalGroup;
    QGroupBox *m_viewGroup;
    QGroupBox *m_languageGroup;
};

} // namespace appimagemanager
