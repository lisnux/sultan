/*
 * escp.cpp
 * Copyright 2017 - ~, Apin <apin.klas@gmail.com>
 *
 * This file is part of Sultan.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "escp.h"

#include <QChar>

using namespace LibPrint;

Escp::Escp(int type, int width10, int width12, int width15):
    mType(type),
    mMaster(0),
    mWidth10(width10),
    mWidth12(width12),
    mWidth15(width15),
    mWidth(width10),
    mCurCol(-1),
    mRestWidth(0),
    mNumLine(0)
{
    init();
}

void Escp::setWidth(int width)
{
    mWidth = width;
}

void Escp::line(const QChar &ch)
{
    for(int i = 0; i < mWidth; i++)
        mData.append(ch);
    newLine();
}

void Escp::newLine(int line)
{
    for(int i = 0; i < line; i++)
        mData.append(QChar(0xA));
    mRestWidth = mWidth;
    mCurCol = 0;
    mNumLine++;
}

void Escp::bold(bool bold)
{
    mData.append(QChar(0x1B));
    mData.append(QChar(0x21));
    if(bold)
        mMaster |= BOLD;
    else
        mMaster &= ~BOLD;
    mData.append(QChar(mMaster));
}

void Escp::doubleHeight(bool value)
{
    mData.append(QChar(0x1B));
    mData.append(QChar(0x21));
    if(value)
        mMaster |= DOUBLE_HEIGHT;
    else
        mMaster &= ~DOUBLE_HEIGHT;
    mData.append(QChar(mMaster));
}

void Escp::doubleWidth(bool /*value*/)
{
    /*mData.append(QChar(0x1B));
    mData.append(QChar(0x21));
    if(value)
        mMaster |= DOUBLE_WIDTH;
    else
        mMaster &= ~DOUBLE_WIDTH;
    mData.append(QChar(mMaster));*/
}

void Escp::cpi10()
{
    mData.append(QChar(0x1B));
    mData.append(QChar(0x21));
    mMaster &= ~CPI;
    mData.append(QChar(mMaster));
    mWidth = mWidth10;
}

void Escp::cpi12()
{
    mData.append(QChar(0x1B));
    mData.append(QChar(0x21));
    mMaster |= CPI;
    mData.append(QChar(mMaster));
    mWidth = mWidth12;
}

void Escp::cpi15()
{
    mData.append(QChar(0x1B));
    mData.append(QChar(0x67));
}

void Escp::column(const QList<int> col)
{
    mColumn = col;
    mCurCol = 0;
}

void Escp::leftText(const QString &str, bool overflow)
{
    int w = mWidth;
    if(mColumn.size() > 0 && mCurCol < mColumn.size()) {
        if(mCurCol >= mColumn.size()) mCurCol = 0;
        w = getCurrentWidth(mCurCol);
        mCurCol++;
    }
    if(mColumn.isEmpty() && overflow) {
        mData.append(str);
    } else {
        if(str.length() > w) {
            QStringRef ref(&str, 0, w);
            mData.append(ref.toString());
        } else {
            QString s = str.leftJustified(w, QChar(0x20));
            mData.append(s);
        }
    }
}

void Escp::centerText(const QString &str, bool overflow)
{
    int w = mWidth;
    if(mColumn.size() > 0 && mCurCol < mColumn.size()) {
        if(mCurCol >= mColumn.size()) mCurCol = 0;
        w = getCurrentWidth(mCurCol);
        mCurCol++;
    }
    if(overflow && mColumn.isEmpty()) {
        writeText(str, w, CENTER);
    } else {
        if(str.length() > w) {
            QStringRef ref(&str, 0, w);
            mData.append(ref.toString());
        } else {
            QString s = str.rightJustified(str.length() + ((w - str.length()) / 2), QChar(0x20));
            s = s.leftJustified(w, QChar(0x20));
            mData.append(s);
        }
    }
}

void Escp::rightText(const QString &str, bool overflow)
{
    int w = mWidth;
    if(mColumn.size() > 0 && mCurCol < mColumn.size()) {
        if(mCurCol >= mColumn.size()) mCurCol = 0;
        w = getCurrentWidth(mCurCol);
        mCurCol++;
    }
    if(overflow && mColumn.isEmpty()) {
        writeText(str, w, RIGHT);
    } else {
        if(str.length() > w) {
            QStringRef ref(&str, 0, w);
            mData.append(ref.toString());
        } else {
            QString s = str.rightJustified(w, QChar(0x20));
            mData.append(s);
        }
    }
}

void Escp::openDrawer()
{
    mData.append(openDrawerCommand());
}

QString Escp::openDrawerCommand()
{
    QString d;
    d.append(QChar(0x1B));
    d.append(QChar(0x70));
    d.append(QChar(0x00));
    d.append(QChar(0x19));
    d.append(QChar(0xfa));
    return d;
}

void Escp::init()
{
    mData.append(QChar(0x1B));
    mData.append(QChar(0x4));
}

int Escp::getCurrentWidth(int col)
{
    if(mColumn.size() > 0) {
        int w = mWidth - mColumn.size();
        if(col < mColumn.size()) {
            return w * mColumn[col] / 100;
        }
    }
    return 0;
}

bool Escp::isTrue(int flag)
{
    return ((mMaster & flag) != 0);
}

void Escp::writeText(const QString &str, int width, int alignment)
{
    const QStringList &list = str.split(QLatin1Char('\n'));
    for(int i = 0; i < list.size(); i++) {
        const QString &val = list.at(i);
        int row = 0;
        int lenght = val.size();
        while(lenght > 0) {
            int l = lenght;
            if(lenght >= width) {
                lenght -= width;
                l = width;
            } else {
                l = lenght;
                lenght = 0;
            }
            QStringRef ref(&val, row * width, l);
            if(alignment == CENTER)
                centerText(ref.toString());
            else
                rightText(ref.toString());
            newLine();
            row++;
        }
    }
}