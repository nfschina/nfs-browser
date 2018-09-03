'use strict';

var isShown = false;

function GetSelection() {
  let t = '';
  if (window.getSelection) {
    t = window.getSelection();
  } else if (document.getSelection) {
    t = document.getSelection();
  } else if (document.selection) {
    t = document.selection.createRange().text;
  }
  return encodeURIComponent(t.toString());
}

function EmptySelection() {
  if (document.selection) {
    document.selection.empty();
  } else if (window.getSelection) {
    window.getSelection().removeAllRanges();
  }
}

function HidePopup () {
  if(isShown) {
    $("#nfspopup").fadeOut(200, function() {
      $(this).remove();
    });
    isShown = false;
  }
}

function showPopup(left, top) {
  let div = document.createElement( 'div' );
  div.id = 'nfspopup';
  div.style.position = "absolute";
  div.style.boxSizing = 'content-box';
  div.style.left = left + 'px';
  div.style.top = top + 'px';
  div.style.padding = "0px";
  div.style.margin = "0px";
  div.style.display = "none";
  div.style.lineHeight = "normal";
  div.style.zIndex = '100000001';
  div.style.fontFamily = "sans-serif";

  let aSearch = document.createElement('a');
  let aCopy = document.createElement('a');

  aSearch.id = "nfssearch";
  aSearch.href = "https://www.baidu.com/s?ie=UTF-8&wd=" + GetSelection();
  aSearch.target = "_blank";
  aSearch.rel = "external";
  aSearch.style = "padding: 4px 4px 4px 4px; margin: 0px; text-decoration:none;";
  aSearch.style.display = "inline";
  aSearch.style.backgroundColor = 'rgb(222, 236, 255)';
  aSearch.style.border = "1px solid #4fa7ff";
  aSearch.style.borderRight = "0px solid #4fa7ff";
  aSearch.style.fontSize = "14px";
  aSearch.style.color = "black";
  aSearch.innerText = "搜索文字"; //chrome.i18n.getMessage("search");
  aSearch.oncontextmenu = function() { return false; }

  aCopy.id = "nfscopy";
  aCopy.href ="javascript:;";
  aCopy.style.padding  = "4px 4px 4px 4px";
  aCopy.style.margin = "0px";
  aCopy.style.cursor = "pointer";
  aCopy.style.display = "inline";
  aCopy.style.textDecoration = "none";
  aCopy.style.fontSize = "14px";
  aCopy.style.color = "black";
  aCopy.style.backgroundColor = 'rgb(222, 236, 255)';
  aCopy.style.border = "1px solid #4fa7ff";
  aCopy.innerText = "复制文字"; //chrome.i18n.getMessage("copy");
  aCopy.onclick = function() {
    document.execCommand('copy');
    EmptySelection();
    HidePopup();
    return false;
  };
  aCopy.oncontextmenu = function() { return false; }

  let aList = [ aSearch, aCopy ];
  for(let i = 0; i < aList.length; i++) {
    aList[i].onmouseover = function() {
      this.style.background = 'rgb(232, 242, 255)';
    };
    aList[i].onmouseout = function() {
      this.style.backgroundColor = 'rgb(222, 236, 255)';
    };
  }

  div.appendChild(aSearch);
  div.appendChild(aCopy);
  document.body.after(div);

  $("#nfspopup").fadeIn(200);

  isShown = true;
}

document.addEventListener("mousedown", function(event) {
  if (event.target.nodeName == "A" &&
      (event.target.id == "nfscopy" || event.target.id == "nfssearch")) {
    return;
  }
  if(event.button == 0) {
    EmptySelection();
  }
  HidePopup();
}, true);

document.addEventListener("mouseup", function(event) {
  chrome.accessPreference.GetBoolean("browser.copySearch", function(enabled) {
    if(enabled) {
      if (event.button == 0 && event.target.id == "nfssearch") {
        EmptySelection();
        HidePopup();
      } else if (event.target.id == "nfscopy") {
        return;
      }
      //Left mouse button
      if(event.button == 0 && event.target.nodeName != "INPUT") {
        let selection = GetSelection();
        if(selection != '') {
          showPopup(event.pageX, event.pageY - 40);
        }
      }
    } else {
    }
  });
}, true);
