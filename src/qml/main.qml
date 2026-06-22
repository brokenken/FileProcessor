import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 900
    height: 650
    visible: true
    title: "File processor"
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        GroupBox {
            title: "Input / Output"
            Layout.fillWidth: true
            GridLayout {
                columns: 3
                anchors.fill: parent
                columnSpacing: 8
                rowSpacing: 8
                Label {
                    text: "Input directory:"
                }
                TextField {
                    id: inputDirectoryField
                    Layout.fillWidth: true
                    text: appController.inputDirectory
                    placeholderText: "Path to input directory"
                    onEditingFinished: appController.inputDirectory = text
                }
                Button {
                    text: "Browse"
                    onClicked: inputDirectoryDialog.open()
                }
                Label {
                    text: "Output directory:"
                }
                TextField {
                    id: outputDirectoryField
                    Layout.fillWidth: true
                    text: appController.outputDirectory
                    placeholderText: "Path to output directory"
                    onEditingFinished: appController.outputDirectory = text
                }
                Button {
                    text: "Browse"
                    onClicked: outputDirectoryDialog.open()
                }
                Label {
                    text: "File mask:"
                }
                TextField {
                    Layout.fillWidth: true
                    text: appController.fileMask
                    placeholderText: "*.txt, *.bin, testFile.bin"
                    onEditingFinished: appController.fileMask = text
                }
                Item {
                    width: 1
                    height: 1
                }
            }
        }
        GroupBox {
            title: "Processing settings"
            Layout.fillWidth: true
            GridLayout {
                columns: 2
                anchors.fill: parent
                columnSpacing: 12
                rowSpacing: 8
                Label {
                    text: "XOR value:"
                }
                TextField {
                    id: xorField
                    Layout.fillWidth: true
                    text: appController.xorHexValue
                    placeholderText: "1234567890ABCDEF"
                    maximumLength: 16
                    inputMethodHints: Qt.ImhPreferUppercase
                    onTextChanged: {
                        text = text.toUpperCase()
                        appController.xorHexValue = text
                    }
                    validator: RegularExpressionValidator {
                        regularExpression: /^[0-9A-Fa-f]{0,16}$/
                    }
                }
                CheckBox {
                    text: "Delete input files after processing"
                    checked: appController.deleteInputFiles
                    onToggled: appController.deleteInputFiles = checked
                }
                CheckBox {
                    text: "Overwrite output files"
                    checked: appController.overwriteOutputFiles
                    onToggled: appController.overwriteOutputFiles = checked
                }
                CheckBox {
                    text: "Timer mode"
                    checked: appController.timerMode
                    onToggled: appController.timerMode = checked
                }
                RowLayout {
                    spacing: 8
                    Label {
                        text: "Poll interval, ms:"
                        enabled: appController.timerMode
                    }
                    SpinBox {
                        from: 100
                        to: 3600000
                        stepSize: 100
                        value: appController.pollIntervalMs
                        enabled: appController.timerMode
                        editable: true
                        onValueModified: appController.pollIntervalMs = value
                    }
                }
            }
        }
        GroupBox {
            title: "Progress"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                Label {
                    Layout.fillWidth: true
                    text: appController.currentFile.length > 0
                        ? "Current file: " + appController.currentFile
                        : "Current file: —"
                    elide: Text.ElideMiddle
                }
                Label {
                    text: "Current file progress: " + appController.currentFileProgress + "%"
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: appController.currentFileProgress
                }
                Label {
                    text: "Total progress: " + appController.totalProgress + "%"
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: appController.totalProgress
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                text: "Start"
                enabled: !appController.running
                onClicked: appController.start()
            }
            Button {
                text: "Pause"
                enabled: appController.running && !appController.paused
                onClicked: appController.pause()
            }
            Button {
                text: "Resume"
                enabled: appController.running && appController.paused
                onClicked: appController.resume()
            }
            Button {
                text: "Stop"
                enabled: appController.running
                onClicked: appController.stop()
            }
            Item {
                Layout.fillWidth: true
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#dddddd"
        }
        Label {
            id: statusLabel
            Layout.fillWidth: true
            text: appController.running
                ? appController.paused ? "Status: paused" : "Status: running"
                : "Status: ready"
        }
        Item {
            Layout.fillHeight: true
        }
    }
    FolderDialog {
        id: inputDirectoryDialog
        title: "Select input directory"
        onAccepted: {
            appController.inputDirectory = selectedFolder.toString().replace("file:///", "")
            inputDirectoryField.text = appController.inputDirectory
        }
    }
    FolderDialog {
        id: outputDirectoryDialog
        title: "Select output directory"
        onAccepted: {
            appController.outputDirectory = selectedFolder.toString().replace("file:///", "")
            outputDirectoryField.text = appController.outputDirectory
        }
    }
    Dialog {
        id: errorDialog
        title: "Error"
        modal: true
        standardButtons: Dialog.Ok
        property string messageText: ""
        Label {
            text: errorDialog.messageText
            wrapMode: Text.WordWrap
            width: 400
        }
    }
    Connections {
        target: appController
        function onErrorOccurred(message) {
            errorDialog.messageText = message
            errorDialog.open()
        }
    }
}