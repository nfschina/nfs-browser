'use strict';

const queryList = [
  "object[classid='CLSID:22D6F312-B0F6-11D0-94AB-0080C74C7E95' i]", //windows media player
  "object[classid='CLSID:6BF52A52-394A-11d3-B153-00C04F79FAA6' i]" //windows media player
];

Element.prototype.insertChildAtIndex = function(child, index) {
  if (!index) index = 0
  if (index >= this.children.length) {
    this.appendChild(child)
  } else {
    this.insertBefore(child, this.children[index])
  }
}

function replaceObject(object) {
  let parent = object.parentNode;

  let nodes = Array.prototype.slice.call(parent.children);
  let index = nodes.indexOf(object);

  let video = document.createElement("video");
  if (!!object["id"])
    video["id"] = object["id"];
  if (!!object["class"])
    video["class"] = object["class"];
  if (!!object["width"])
    video["width"] = object["width"];
  if (!!object["height"])
    video["height"] = object["height"];

  video["style"] = object["style"];

  let param = undefined;
  let value = undefined;

  param = object.querySelector("param[name='filename' i]");
  if (!!param)
    video["src"] = param.getAttribute("value");

  param = object.querySelector("param[name='url' i]");
  if (!!param)
    video["src"] = param.getAttribute("value");

  param = object.querySelector("param[name='autostart' i]");
  if (!!param) {
    value = param.getAttribute("value");
    if (true === value || 1 === value || "true" === value || "1" === value)
      video.setAttribute("autoplay", "");
  } else {
    video.setAttribute("autoplay", "");
  }

  param = object.querySelector("param[name='showcontrols' i]");
  if (!!param) {
    value = param.getAttribute("value");
    if (true === value || 1 === value || "true" === value || "1" === value)
      video.setAttribute("controls", "");
  } else {
    video.setAttribute("controls", "");
  }

  parent.removeChild(object);
  parent.insertChildAtIndex(video, index);
}

if (window.location.host == "www.bankcomm.com") {
  var search_form = document.getElementById('wangyindenglu');
  if (search_form) {
    search_form.id = "dengluwangyin";
    if (search_form.id == 'dengluwangyin') {
      search_form.setAttribute("href","http://www.bankcomm.com/BankCommSite/getDispatcherUrl.do?reqType=31");
    }
  }
}

if (window.location.host == "perbank.abchina.com") {
  var obj1 = document.getElementById("ABCPrintCtl4RA1");
  if (obj1) {
    obj1.removeAttribute("classid");
    obj1.setAttribute("type", "application/x-itst-activex");
    obj1.setAttribute("clsid", "{5DA34F59-FBFF-4666-99F5-599CD7B9A640}");
  }

  var obj2 = document.getElementById("g_objClassFactory");
  if (obj2) {
    obj2.removeAttribute("classid");
    obj2.setAttribute("type", "application/x-itst-activex");
    obj2.setAttribute("clsid", "{884e2049-217d-11da-b2a4-000e7bbb2b09}");
  }
}

for (let j = 0; j < queryList.length; ++j) {
  let list = document.querySelectorAll(queryList[j]);
  const count = list.length;
  for (let i = 0; i < count; ++i) {
    replaceObject(list[i]);
  }
}