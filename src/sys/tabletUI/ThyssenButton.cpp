/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <assert.h>
#include <stdio.h>

#include <QPushButton>
#include <QLabel>
#include <QPixmap>

#include "ThyssenButton.h"
#include "TUIApplication.h"
#include <net/tokenbuffer.h>

/// Constructor
ThyssenButton::ThyssenButton(int id, int type, QWidget *w, int parent, QString name)
    : TUIElement(id, type, w, parent, name)
{
    std::cerr << "name: " << name.toStdString() << std::endl;
    number = name.toInt();
    std::cerr << "number: " << number << std::endl;
    QPushButton *b = new QPushButton(w);
    if (name.contains("."))
    {
        QPixmap pm(name);
        if (pm.isNull())
        {
            QString covisedir = QString(getenv("COVISEDIR"));
            QPixmap pm(covisedir + "/" + name);
            QPixmap qm(covisedir + "/icons/" + name);
            if (pm.isNull() && qm.isNull())
            {
                b->setText(name);
            }
            else if (pm.isNull())
            {
                b->setIcon(qm);
            }
            else
            {
                b->setIcon(pm);
            }
        }
        else
        {
            b->setIcon(pm);
        }
    }
    else
        b->setText(name);

    ThyssenPanel::instance()->buttons.push_back(this);
    //b->setFixedSize(b->sizeHint());
    widget = b;
    connect(b, SIGNAL(pressed()), this, SLOT(pressed()));
    connect(b, SIGNAL(released()), this, SLOT(released()));
}

/// Destructor
ThyssenButton::~ThyssenButton()
{
    delete widget;
}

void ThyssenButton::update(uint8_t bs,bool wasPressed)
{
    bool currentState = (bs == number);
    std::cerr << (int)bs << " wasPr" << wasPressed <<  oldState << currentState << std::endl;
    if(bs == number || oldState == true)
    {
        covise::TokenBuffer tb;
        tb << ID;
        if(currentState && wasPressed)
        {
            tb << TABLET_PRESSED;
        }
        else
        {
            currentState = false;
            tb << TABLET_RELEASED;
        }
        TUIMainWindow::getInstance()->send(tb);
        oldState = currentState;
    }
}

void ThyssenButton::pressed()
{
    covise::TokenBuffer tb;
    tb << ID;
    tb << TABLET_PRESSED;
    TUIMainWindow::getInstance()->send(tb);
}

void ThyssenButton::released()
{
    covise::TokenBuffer tb;
    tb << ID;
    tb << TABLET_RELEASED;
    TUIMainWindow::getInstance()->send(tb);
}

void ThyssenButton::setSize(int w, int h)
{
    QPushButton *b = (QPushButton *)widget;
    b->setIconSize(QSize(w, h)); /* Max size of icons, smaller icons will not be scaled up */
    b->setFixedSize(b->sizeHint());
}

const char *ThyssenButton::getClassName() const
{
    return "ThyssenButton";
}

void ThyssenButton::setLabel(QString textl)
{
    TUIElement::setLabel(textl);
    if (QAbstractButton* b = qobject_cast<QAbstractButton*>(widget))
    {
        b->setText(textl);
    }
}
