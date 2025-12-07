/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QStackedWidget *stackedWidget;
    QWidget *page_3;
    QGridLayout *gridLayout_3;
    QWidget *verticalWidget;
    QGridLayout *gridLayout_2;
    QLabel *label_2;
    QLineEdit *passLineEdit;
    QLineEdit *userLineEdit;
    QLabel *label;
    QPushButton *loginButton;
    QWidget *page_4;
    QVBoxLayout *verticalLayout;
    QWidget *verticalWidget_2;
    QVBoxLayout *verticalLayout_2;
    QPushButton *getButton;
    QPushButton *postButton;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1078, 654);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        QPalette palette;
        QBrush brush(QColor(0, 0, 0, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::WindowText, brush);
        QBrush brush1(QColor(220, 220, 220, 255));
        brush1.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush1);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Text, brush);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::ButtonText, brush);
        QBrush brush2(QColor(240, 240, 240, 255));
        brush2.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush2);
        QBrush brush3(QColor(255, 255, 255, 255));
        brush3.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Window, brush3);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::AlternateBase, brush3);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::WindowText, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush1);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Text, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::ButtonText, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush2);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Window, brush3);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::AlternateBase, brush3);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush1);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, brush3);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window, brush3);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::AlternateBase, brush3);
        stackedWidget->setPalette(palette);
        page_3 = new QWidget();
        page_3->setObjectName("page_3");
        gridLayout_3 = new QGridLayout(page_3);
        gridLayout_3->setObjectName("gridLayout_3");
        verticalWidget = new QWidget(page_3);
        verticalWidget->setObjectName("verticalWidget");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(50);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(verticalWidget->sizePolicy().hasHeightForWidth());
        verticalWidget->setSizePolicy(sizePolicy1);
        verticalWidget->setMaximumSize(QSize(400, 200));
        gridLayout_2 = new QGridLayout(verticalWidget);
        gridLayout_2->setObjectName("gridLayout_2");
        label_2 = new QLabel(verticalWidget);
        label_2->setObjectName("label_2");

        gridLayout_2->addWidget(label_2, 2, 0, 1, 1, Qt::AlignmentFlag::AlignHCenter);

        passLineEdit = new QLineEdit(verticalWidget);
        passLineEdit->setObjectName("passLineEdit");

        gridLayout_2->addWidget(passLineEdit, 3, 0, 1, 1, Qt::AlignmentFlag::AlignHCenter);

        userLineEdit = new QLineEdit(verticalWidget);
        userLineEdit->setObjectName("userLineEdit");
        userLineEdit->setMaximumSize(QSize(400, 16777215));

        gridLayout_2->addWidget(userLineEdit, 1, 0, 1, 1, Qt::AlignmentFlag::AlignHCenter);

        label = new QLabel(verticalWidget);
        label->setObjectName("label");

        gridLayout_2->addWidget(label, 0, 0, 1, 1, Qt::AlignmentFlag::AlignHCenter);

        loginButton = new QPushButton(verticalWidget);
        loginButton->setObjectName("loginButton");

        gridLayout_2->addWidget(loginButton, 4, 0, 1, 1, Qt::AlignmentFlag::AlignHCenter);


        gridLayout_3->addWidget(verticalWidget, 1, 1, 1, 1);

        stackedWidget->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName("page_4");
        verticalLayout = new QVBoxLayout(page_4);
        verticalLayout->setObjectName("verticalLayout");
        verticalWidget_2 = new QWidget(page_4);
        verticalWidget_2->setObjectName("verticalWidget_2");
        verticalWidget_2->setMinimumSize(QSize(200, 100));
        verticalLayout_2 = new QVBoxLayout(verticalWidget_2);
        verticalLayout_2->setObjectName("verticalLayout_2");
        getButton = new QPushButton(verticalWidget_2);
        getButton->setObjectName("getButton");

        verticalLayout_2->addWidget(getButton);

        postButton = new QPushButton(verticalWidget_2);
        postButton->setObjectName("postButton");

        verticalLayout_2->addWidget(postButton);


        verticalLayout->addWidget(verticalWidget_2, 0, Qt::AlignmentFlag::AlignHCenter|Qt::AlignmentFlag::AlignVCenter);

        stackedWidget->addWidget(page_4);

        gridLayout->addWidget(stackedWidget, 1, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Password", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Username", nullptr));
        loginButton->setText(QCoreApplication::translate("MainWindow", "Login", nullptr));
        getButton->setText(QCoreApplication::translate("MainWindow", "Get file", nullptr));
        postButton->setText(QCoreApplication::translate("MainWindow", "Post file", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
