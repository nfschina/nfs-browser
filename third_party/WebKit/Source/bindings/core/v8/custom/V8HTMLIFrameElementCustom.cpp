#include "bindings/core/v8/V8HTMLIFrameElement.h"

namespace blink {

void V8HTMLIFrameElement::documentAttributeGetterCustom(
  const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();

  HTMLIFrameElement* impl = V8HTMLIFrameElement::toImpl(holder);

  // FIXME: Can not be exclude all exception cases. E.g onload="document.getElementById("iframe_id").document.getElementById("other_id")"
  if (impl && impl->hasAttribute("onload") && impl->getAttribute("onload").getString().contains("document.")) {
    v8SetReturnValueFast(info, WTF::getPtr(impl->document()), impl);
    return;
  }

  // Perform a security check for the returned object.
  ExceptionState exceptionState(info.GetIsolate(), ExceptionState::GetterContext, "HTMLIFrameElement", "contentDocument");
  if (!BindingSecurity::shouldAllowAccessTo(currentDOMWindow(info.GetIsolate()), WTF::getPtr(impl->contentDocument()), exceptionState)) {
    v8SetReturnValueNull(info);
    return;
  }

  //return contentDocument()
  v8SetReturnValueFast(info, WTF::getPtr(impl->contentDocument()), impl);
}

}
