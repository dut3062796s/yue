// Copyright 2016 Cheng Zhao. All rights reserved.
// Use of this source code is governed by the license that can be found in the
// LICENSE file.

#include "nativeui/mac/events_handler.h"

#include <objc/objc-runtime.h>

#include "base/logging.h"
#include "nativeui/events/event.h"
#include "nativeui/mac/view_mac.h"

namespace nu {

namespace {

bool NUInjected(NSView* self, SEL _cmd) {
  return true;
}

void OnMouseEvent(NSView* self, SEL _cmd, NSEvent* event) {
  DCHECK([self respondsToSelector:@selector(shell)])
      << "Handler called for view other than NUView";
  View* view = [self shell];
  DCHECK(view);

  // Emit the event to View.
  bool prevent_default = false;
  MouseEvent mouse_event(event, self);
  if (mouse_event.type == EventType::MouseDown)
    prevent_default = view->on_mouse_down.Emit(view, mouse_event);
  else if (mouse_event.type == EventType::MouseUp)
    prevent_default = view->on_mouse_up.Emit(view, mouse_event);

  // Transfer the event to super class.
  if (!prevent_default) {
    auto super_impl = reinterpret_cast<void (*)(NSView*, SEL, NSEvent*)>(
        [[self superclass] instanceMethodForSelector:_cmd]);
    super_impl(self, _cmd, event);
  }
}

void OnKeyEvent(NSView* self, SEL _cmd, NSEvent* event) {
  DCHECK([self respondsToSelector:@selector(shell)])
      << "Handler called for view other than NUView";
  View* view = [self shell];
  DCHECK(view);

  // Emit the event to View.
  bool prevent_default = false;
  KeyEvent key_event(event, self);
  if (key_event.type == EventType::KeyDown)
    prevent_default = view->on_key_down.Emit(view, key_event);
  else if (key_event.type == EventType::KeyUp)
    prevent_default = view->on_key_up.Emit(view, key_event);
  else
    NOTREACHED();

  // Transfer the event to super class.
  if (!prevent_default) {
    auto super_impl = reinterpret_cast<void (*)(NSView*, SEL, NSEvent*)>(
        [[self superclass] instanceMethodForSelector:_cmd]);
    super_impl(self, _cmd, event);
  }
}

BOOL AcceptsFirstResponder(NSView* self, SEL _cmd) {
  return YES;
}

}  // namespace

bool IsNUView(id view) {
  return [view respondsToSelector:@selector(shell)];
}

bool EventHandlerInstalled(Class cl) {
  return class_getClassMethod(cl, @selector(IsNUView)) != nullptr;
}

void AddMouseEventHandlerToClass(Class cl) {
  class_addMethod(cl, @selector(nuInjected), (IMP)NUInjected, "B@:");
  class_addMethod(cl, @selector(mouseDown:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(rightMouseDown:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(otherMouseDown:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(mouseUp:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(rightMouseUp:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(otherMouseUp:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(mouseMoved:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(mouseEntered:), (IMP)OnMouseEvent, "v@:@");
  class_addMethod(cl, @selector(mouseExited:), (IMP)OnMouseEvent, "v@:@");
}

void AddKeyEventHandlerToClass(Class cl) {
  class_addMethod(cl, @selector(keyDown:), (IMP)OnKeyEvent, "v@:@");
  class_addMethod(cl, @selector(keyUp:), (IMP)OnKeyEvent, "v@:@");
  class_addMethod(cl, @selector(flagsChanged:), (IMP)OnKeyEvent, "v@:@");
}

void AddViewMethodsToClass(Class cl) {
  class_addMethod(cl, @selector(acceptsFirstResponder),
                  (IMP)AcceptsFirstResponder, "B@:");
}

}  // namespace nu
