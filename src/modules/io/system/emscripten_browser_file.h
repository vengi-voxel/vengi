#pragma once

#include <string>
#include <string_view>
#include <emscripten.h>

#define _EM_JS_INLINE(ret, c_name, js_name, params, code)                          \
  extern "C" {                                                                     \
  ret c_name params EM_IMPORT(js_name);                                            \
  EMSCRIPTEN_KEEPALIVE                                                             \
  __attribute__((section("em_js"), aligned(1))) inline char __em_js__##js_name[] = \
    #params "<::>" code;                                                           \
  }

#define EM_JS_INLINE(ret, name, params, ...) _EM_JS_INLINE(ret, name, name, params, #__VA_ARGS__)

namespace emscripten_browser_file {

/////////////////////////////////// Interface //////////////////////////////////

using upload_handler = void(*)(std::string const&, std::string const&, std::string_view buffer, void*);

inline void upload(std::string const &accept_types, upload_handler callback, void *callback_data = nullptr);
inline void download(std::string const &filename, std::string const &mime_type, std::string_view buffer);

///////////////////////////////// Implementation ///////////////////////////////

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
EM_JS_INLINE(void, upload, (char const *accept_types, upload_handler callback, void *callback_data), {
  /// Prompt the browser to open the file selector dialogue, and pass the file to the given handler
  /// Accept-types are in the format ".png,.jpeg,.jpg" as per https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/accept
  /// Upload handler callback signature is:
  ///   void my_handler(std::string const &filename, std::string const &mime_type, std::string_view buffer, void *callback_data = nullptr);
  globalThis["open_file"] = function(e) {
    const file_reader = new FileReader();
    file_reader.onload = (event) => {
      const uint8Arr = new Uint8Array(event.target.result);
      const data_ptr = Module["_malloc"](uint8Arr.length);
      const data_on_heap = new Uint8Array(Module["HEAPU8"].buffer, data_ptr, uint8Arr.length);
      data_on_heap.set(uint8Arr);
      Module["ccall"]('upload_file_return', 'number', ['string', 'string', 'number', 'number', 'number', 'number'], [event.target.filename, event.target.mime_type, data_on_heap.byteOffset, uint8Arr.length, callback, callback_data]);
      Module["_free"](data_ptr);
    };
    file_reader.filename = e.target.files[0].name;
    file_reader.mime_type = e.target.files[0].type;
    file_reader.readAsArrayBuffer(e.target.files[0]);
  };
  var file_selector = document.createElement('input');
  file_selector.setAttribute('type', 'file');
  file_selector.setAttribute('onchange', 'globalThis["open_file"](event)');
  /// The 'cancel' event is fired when the user cancels the currently open dialog.
  /// In this case, the upload handler will get the empty string_view.
  /// See https://developer.mozilla.org/en-US/docs/Web/API/HTMLElement/cancel_event
  file_selector.addEventListener('cancel', () => {
    Module["ccall"]('upload_file_return', 'number', ['string', 'string', 'number', 'number', 'number', 'number'], ["", "", 0, 0, callback, callback_data]);
  });
  file_selector.setAttribute('accept', UTF8ToString(accept_types));
  /// file_selector.click() approach doesn't work in Safari (tested with native desktop v. 17.5 and iPhone/iPad simulators).
  /// It seems that the Safari browser limits programmatical clicking in our case.
  /// As a workaround, we create <dialog> where the user manually clicks on <input>.
  var is_safari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);
  if (is_safari) {
    var dialog = document.createElement('dialog');
    dialog.setAttribute('id', 'EmJsFileDialog');
    var desc = document.createElement('p');
    desc.innerText = 'Please choose a file. Allowed extension(s): ' + UTF8ToString(accept_types);
    dialog.appendChild(desc);
    /// We should recreate <dialog> every call; it is the most natural way to reset input.value.
    /// Otherwise, if the user re-selects the same file, it triggers the 'cancel' event instead of 'onchange'.
    file_selector.setAttribute('onclick', 'var dg = document.getElementById("EmJsFileDialog"); dg.close(); dg.remove()');
    dialog.appendChild(file_selector);
    document.body.append(dialog);
    dialog.showModal();
  } else {
    /// Not a Safari browser, so file_selector.click() is ok.
    file_selector.click();
  }
});
#pragma GCC diagnostic pop

inline void upload(std::string const &accept_types, upload_handler callback, void *callback_data) {
  /// C++ wrapper for javascript upload call
  upload(accept_types.c_str(), callback, callback_data);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
EM_JS_INLINE(void, download, (char const *filename, char const *mime_type, void const *buffer, size_t buffer_size), {
  /// Offer a buffer in memory as a file to download, specifying download filename and mime type
  var a = document.createElement('a');
  a.download = UTF8ToString(filename);
  a.href = URL.createObjectURL(new Blob([new Uint8Array(Module["HEAPU8"].buffer, buffer, buffer_size)], {type: UTF8ToString(mime_type)}));
  a.click();
});
#pragma GCC diagnostic pop

inline void download(std::string const &filename, std::string const &mime_type, std::string_view buffer) {
  /// C++ wrapper for javascript download call, accepting a string_view
  download(filename.c_str(), mime_type.c_str(), buffer.data(), buffer.size());
}

namespace {

extern "C" {

EMSCRIPTEN_KEEPALIVE inline int upload_file_return(char const *filename, char const *mime_type, char *buffer, size_t buffer_size, upload_handler callback, void *callback_data);

EMSCRIPTEN_KEEPALIVE inline int upload_file_return(char const *filename, char const *mime_type, char *buffer, size_t buffer_size, upload_handler callback, void *callback_data) {
  /// Load a file - this function is called from javascript when the file upload is activated

  /// The file was not uploaded.
  /// We must process this case separately because std::string_view(nullptr, 0) results in UB.
  /// <The behavior is undefined if [s, s + count) is not a valid range
  /// (even though the constructor may not access any of the elements of this range)>
  /// https://en.cppreference.com/w/cpp/string/basic_string_view/basic_string_view
  if (! buffer || buffer_size == 0) {
    callback(filename, mime_type, std::string_view(), callback_data);
    return 1;
  }
  /// Ok
  callback(filename, mime_type, {buffer, buffer_size}, callback_data);
  return 1;
}

}

}

}
