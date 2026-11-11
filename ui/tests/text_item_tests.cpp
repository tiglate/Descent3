/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// UIRes.cpp is compiled directly; drawing entry points are never reached by
// these tests and are satisfied by stubs below.
#include "uires.h"
#include "uidraw.h"
#include "pserror.h"

#include <gtest/gtest.h>

#include <cstring>

float UIFrameTime = 0.0f;
void ui_DrawRect(ddgr_color, int, int, int, int) {}
void ui_DrawBitmap(UIBitmapItem *, int, int, uint8_t) {}
void ui_DrawString(ddgr_color, int, int, const char *) {}
void ui_DrawSetFont(int) {}
void ui_DrawSetAlpha(uint8_t) {}
int ui_GetTextWidth(const char *) { return 0; }
void ui_SetCharAlpha(uint8_t) {}
int ui_GetTextHeight(const char *) { return 0; }
int ui_GetBitmapWidth(UIBitmapItem *) { return 0; }
void ui_DrawSetTextType(int) {}
int ui_GetBitmapHeight(UIBitmapItem *) { return 0; }
void Error(const char *, ...) { ADD_FAILURE() << "Error() called"; }

TEST(UITextItemCopy, CopiesText) {
  UITextItem source("hello");
  UITextItem copy;
  copy = source;
  EXPECT_STREQ(copy.GetBuffer(), "hello");
  EXPECT_NE(copy.GetBuffer(), source.GetBuffer());
}

TEST(UITextItemCopy, SelfAssignmentKeepsText) {
  UITextItem item("keep me");
  const UITextItem &alias = item;
  item = alias;
  EXPECT_STREQ(item.GetBuffer(), "keep me");
}

TEST(UITextItemCopy, DefaultConstructedSourceCopiesAsEmpty) {
  UITextItem empty;
  UITextItem target("old");
  target = empty;
  ASSERT_NE(target.GetBuffer(), nullptr);
  EXPECT_STREQ(target.GetBuffer(), "");
}

TEST(UITextItemCopy, CopyUIItemDuplicatesDefaultAndEmptyItems) {
  UITextItem empty;
  UIItem *copy = empty.CopyUIItem();
  ASSERT_NE(copy, nullptr);
  delete copy;
  UITextItem blank("");
  UIItem *copy2 = blank.CopyUIItem();
  EXPECT_STREQ(static_cast<UITextItem *>(copy2)->GetBuffer(), "");
  delete copy2;
}

TEST(UISnazzyTextItemCopy, SelfAndRepeatedAssignment) {
  UISnazzyTextItem a(0, "snazzy");
  const UISnazzyTextItem &alias = a;
  a = alias;
  EXPECT_STREQ(a.GetBuffer(), "snazzy");
  UISnazzyTextItem b(0, "other");
  for (int i = 0; i < 3; ++i)
    b = a;
  EXPECT_STREQ(b.GetBuffer(), "snazzy");
}
