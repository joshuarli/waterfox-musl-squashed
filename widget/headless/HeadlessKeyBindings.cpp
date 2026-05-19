/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HeadlessKeyBindings.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Maybe.h"
#include "mozilla/NativeKeyBindingsType.h"
#include "mozilla/WritingModes.h"

namespace mozilla {
namespace widget {

HeadlessKeyBindings& HeadlessKeyBindings::GetInstance() {
  static UniquePtr<HeadlessKeyBindings> sInstance;
  if (!sInstance) {
    sInstance.reset(new HeadlessKeyBindings());
    ClearOnShutdown(&sInstance);
  }
  return *sInstance;
}

nsresult HeadlessKeyBindings::AttachNativeKeyEvent(
    WidgetKeyboardEvent& aEvent) {
  // Stub for non-mac platforms.
  return NS_OK;
}

static void AppendCommand(nsTArray<CommandInt>& aCommands, Command aCommand) {
  if (aCommand != Command::DoNothing) {
    aCommands.AppendElement(static_cast<CommandInt>(aCommand));
  }
}

void HeadlessKeyBindings::GetEditCommands(
    NativeKeyBindingsType aType, const WidgetKeyboardEvent& aEvent,
    const Maybe<WritingMode>& aWritingMode, nsTArray<CommandInt>& aCommands) {
  if (aEvent.IsAlt()) {
    return;
  }

  const bool accel = aEvent.IsControl() || aEvent.IsMeta();
  const bool shift = aEvent.IsShift();
  const KeyNameIndex keyNameIndex =
      aWritingMode.isSome() ? aEvent.GetRemappedKeyNameIndex(aWritingMode.ref())
                            : aEvent.mKeyNameIndex;

  if (keyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    switch (aEvent.PseudoCharCode()) {
      case 'a':
      case 'A':
        if (accel) {
          AppendCommand(aCommands, Command::SelectAll);
        }
        return;
      case 'c':
      case 'C':
        if (accel && !shift) {
          AppendCommand(aCommands, Command::Copy);
        }
        return;
      case 'u':
      case 'U':
        if (aType == NativeKeyBindingsType::SingleLineEditor && accel &&
            !shift) {
          AppendCommand(aCommands, Command::DeleteToBeginningOfLine);
        }
        return;
      case 'v':
      case 'V':
        if (accel && !shift) {
          AppendCommand(aCommands, Command::Paste);
        }
        return;
      case 'x':
      case 'X':
        if (accel && !shift) {
          AppendCommand(aCommands, Command::Cut);
        }
        return;
      case 'y':
      case 'Y':
        if (accel && !shift) {
          AppendCommand(aCommands, Command::HistoryRedo);
        }
        return;
      case 'z':
      case 'Z':
        if (accel) {
          AppendCommand(aCommands,
                        shift ? Command::HistoryRedo : Command::HistoryUndo);
        }
        return;
      case '/':
        if (accel && !shift) {
          AppendCommand(aCommands, Command::SelectAll);
        }
        return;
      default:
        return;
    }
  }

  switch (keyNameIndex) {
    case KEY_NAME_INDEX_Insert:
      if (accel && !shift) {
        AppendCommand(aCommands, Command::Copy);
      } else if (shift && !accel) {
        AppendCommand(aCommands, Command::Paste);
      }
      break;
    case KEY_NAME_INDEX_Delete:
      if (shift) {
        AppendCommand(aCommands, Command::Cut);
      } else {
        AppendCommand(aCommands, accel ? Command::DeleteWordForward
                                       : Command::DeleteCharForward);
      }
      break;
    case KEY_NAME_INDEX_Backspace:
      AppendCommand(aCommands, accel ? Command::DeleteWordBackward
                                     : Command::DeleteCharBackward);
      break;
    case KEY_NAME_INDEX_ArrowLeft:
      AppendCommand(aCommands,
                    accel ? (shift ? Command::SelectWordPrevious
                                   : Command::WordPrevious)
                          : (shift ? Command::SelectCharPrevious
                                   : Command::CharPrevious));
      break;
    case KEY_NAME_INDEX_ArrowRight:
      AppendCommand(aCommands,
                    accel ? (shift ? Command::SelectWordNext
                                   : Command::WordNext)
                          : (shift ? Command::SelectCharNext
                                   : Command::CharNext));
      break;
    case KEY_NAME_INDEX_ArrowUp:
      AppendCommand(aCommands,
                    shift ? Command::SelectLinePrevious : Command::LinePrevious);
      break;
    case KEY_NAME_INDEX_ArrowDown:
      AppendCommand(aCommands,
                    shift ? Command::SelectLineNext : Command::LineNext);
      break;
    case KEY_NAME_INDEX_Home:
      AppendCommand(aCommands,
                    accel ? (shift ? Command::SelectTop : Command::MoveTop)
                          : (shift ? Command::SelectBeginLine
                                   : Command::BeginLine));
      break;
    case KEY_NAME_INDEX_End:
      AppendCommand(aCommands,
                    accel ? (shift ? Command::SelectBottom
                                   : Command::MoveBottom)
                          : (shift ? Command::SelectEndLine : Command::EndLine));
      break;
    case KEY_NAME_INDEX_PageUp:
      AppendCommand(aCommands,
                    shift ? Command::SelectPageUp : Command::MovePageUp);
      break;
    case KEY_NAME_INDEX_PageDown:
      AppendCommand(aCommands,
                    shift ? Command::SelectPageDown : Command::MovePageDown);
      break;
    default:
      break;
  }
}

}  // namespace widget
}  // namespace mozilla
