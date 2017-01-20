/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKSIM_XBUS_MESSAGES_H
#define BLACKSIM_XBUS_MESSAGES_H

//! \file

#include "drawable.h"
#include "command.h"
#include <vector>
#include <string>
#include <array>
#include <algorithm>

namespace XBus
{

    /*!
     * Class representing a single line of text to be drawn in a message box.
     */
    struct CMessage
    {
        //! Constructor.
        CMessage(const std::string &text, float r = 1, float g = 1, float b = 1) : m_text(text), m_rgb{{ r, g, b }} {}

        //! Text.
        std::string m_text;

        //! Color.
        std::array<float, 3> m_rgb;
    };

    /*!
     * Class for drawing a gray box with text messages.
     */
    class CMessageBox : public CDrawable
    {
    public:
        //! Constructor.
        //! \param left Number of "virtual pixels" between screen left edge and box left edge.
        //! \param right Number of "virtual pixels" between screen right edge and box right edge.
        //! \param top Number of "virtual pixels" between screen top edge and box top edge.
        CMessageBox(int left, int right, int top) : CDrawable(xplm_Phase_Window, true),
            c_boxLeft(left), c_boxRight(c_screenWidth - right), c_boxTop(c_screenHeight - top) {}

        //! Set messages to draw in message box, from a pair of iterators.
        template <typename Iterator>
        void setMessages(Iterator begin, Iterator end)
        {
            m_messages.clear();
            std::copy(begin, end, std::back_inserter(m_messages));
        }

        //! Set whether to draw a small arrow at the bottom of the box.
        void enableArrows(bool up, bool down)
        {
            m_upArrow = up;
            m_downArrow = down;
        }

        //! Returns the maximum number of characters per line.
        int maxLineLength() const;

    protected:
        virtual void draw() override;

    private:
        std::vector<CMessage> m_messages;
        bool m_upArrow = false;
        bool m_downArrow = false;
        const int c_boxLeft = 0;
        const int c_boxRight = 0;
        const int c_boxTop = 0;
        constexpr static int c_screenWidth = 1024;
        constexpr static int c_screenHeight = 768;
    };

    /*!
     * Class which builds upon CMessageBox with a scrollback buffer and commands for user control.
     */
    class CMessageBoxControl
    {
    public:
        //! \copydoc CMessageBox::CMessageBox
        CMessageBoxControl(int left, int right, int top);

        //! Add a new message to the bottom of the list.
        void addMessage(const CMessage &message);

        //! \copydoc XBus::CMessageBox::maxLineLength
        int maxLineLength() const { return m_messageBox.maxLineLength(); }

    private:
        void show() { m_messageBox.show(); m_visible = true; }
        void hide() { m_messageBox.hide(); m_visible = false; }
        void toggle() { if (m_visible) { hide(); } else { show(); } }
        void scrollUp();
        void scrollDown();
        void scrollToTop();
        void scrollToBottom();
        void updateVisibleLines();

        bool m_visible = false;
        std::vector<CMessage> m_messages;
        size_t m_position = 0;
        const size_t c_maxVisibleLines = 8;
        const size_t c_maxTotalLines = 1024;
        CMessageBox m_messageBox;

        CCommand m_showCommand;
        CCommand m_hideCommand;
        CCommand m_toggleCommand;
        CCommand m_scrollUpCommand;
        CCommand m_scrollDownCommand;
        CCommand m_scrollToTopCommand;
        CCommand m_scrollToBottomCommand;
        CCommand m_debugCommand;
    };

}

#endif
