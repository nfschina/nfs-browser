  function globalStruct() {
    this.targetVideo = null,
    this.currentTarget = null,
    this.mouseHover = null,
    this.videoProcessTimer = null,
    this.timer = null,
    this.runTimer = null,
    this.showStatus = 0,
    this.isVideoPopupWindow = 0
  }

  function saveOriStyle(e) {
    //zhangyq
    if (e.hasAttribute("nfs_style")) {
      return;
    }
    var t = e.getAttribute("style");
    null != t && e.setAttribute("nfs_style", t),
    (null == t) && e.setAttribute("nfs_style", "")
  }

  function removeTmpStyle(e) {
    var t = e.getAttribute("nfs_style");
    null != t && (e.setAttribute("style", t), e.removeAttribute("nfs_style")),
    (null == t) && e.setAttribute("style", "") && e.removeAttribute("style")
  }

  function hideBodyChildren(t) {
    for (var o = document.body.children, i = 0; i < o.length; i++)
      if ("function" != typeof o[i].nodeType && 1 == o[i].nodeType && (o[i].nodeName.toLowerCase(), o[i] != t && o[i] != gs.targetVideo)) {
        var n = window.getComputedStyle(o[i], null);
        ("none" != n.getPropertyValue("display") || "" != n.getPropertyValue("position")) && (saveOriStyle(o[i]), o[i].style.visibility = "hidden", o[i].style.display="none", setAttribute(o[i], "nfs_display", "none"))
      }
  }

  function addDisplayNone(t) {
    var t = t.children;
    if ("undefined" == typeof t)
      return !0;
    for (var o = 0; o < t.length; o++)
      if (t[o].nodeName.toLowerCase(), 1 == t[o].nodeType && "object" != t[o].nodeName.toLowerCase() && "embed" != t[o].nodeName.toLowerCase() && "video" != t[o].nodeName.toLowerCase())
        if ("block" != t[o].getAttribute("nfs_display") && t[o] != gs.currentTarget) {
          //[zhangyq] Filter out player control element.
          if ((location.hostname == "www.iqiyi.com" && (t[o] == document.querySelector("div[data-player-hook='plgcontainer']") ||
              t[o] == document.querySelector("div[data-cupid='container']"))) ||
              (location.hostname == "www.bilibili.com" && (t[o] == document.querySelector("div .bilibili-player-video-control"))) ||
              (location.hostname == "v.youku.com" && t[o] == document.querySelector("div .h5player-dashboard")) ||
              (location.hostname.includes("tudou.com") && t[o] == document.querySelector("div .h5player-dashboard")) ||
              (location.hostname == "v.ifeng.com" && t[o] == document.querySelector("div .videoControl")) ||
              ((navigator.platform =="Win32" || navigator.platform =="Windows") && location.hostname == "v.qq.com" && t[o] == document.querySelector("txpdiv .txp_bottom"))) {
            if ((location.hostname == "www.bilibili.com" && t[o] == document.querySelector("div .bilibili-player-video-control")) ||
                ((navigator.platform =="Win32" || navigator.platform =="Windows") && location.hostname == "v.qq.com" && t[o] == document.querySelector("txpdiv .txp_bottom"))) {
              var n = window.getComputedStyle(t[o], null);
              ("none" != n.getPropertyValue("display")) && (saveOriStyle(t[o]), t[o].style.position = "fixed",
                  t[o].style.left = "0px", t[o].style.bottom = "0px", t[o].style.zIndex = "2147483647");
            }
            if (location.hostname == "v.ifeng.com" && t[o] == document.querySelector("div .videoControl")) {
              var n = window.getComputedStyle(t[o], null);
              ("none" != n.getPropertyValue("display")) && (saveOriStyle(t[o]), t[o].style.position = "absolute",
                  t[o].style.left = "0px", t[o].style.bottom = "0px", t[o].style.zIndex = "2147483647");
            }
            continue;
          }
          var n = window.getComputedStyle(t[o], null);
          ("none" != n.getPropertyValue("display") || "" != n.getPropertyValue("position")) && (saveOriStyle(t[o]), t[o].style.visibility = "hidden", t[o].style.display = "none",
          setAttribute(t[o], "nfs_display", "none"));
        } else
          addDisplayNone(t[o])
  }

  function parseIframe(t) {
    return "body" == t.nodeName.toLowerCase() ? t : ("iframe" == t.nodeName.toLowerCase() && setAttribute(t, "nfs_display", "block"), "body" != t.parentNode.nodeName.toLowerCase() ? (t = t.parentNode, saveOriStyle(t), t.style.padding = "0px", t.style.transform = "inherit", t.style.position = "static", setAttribute(t, "nfs_display", "block"), parseIframe(t)) : ("" != t.style.position && "static" != t.style.position && (setAttribute(t, "nfs_postion", t.style.position), saveOriStyle(t), t.style.zIndex = "9999999"), t))
  }

  function getTarget(e) {
    for (var t = new Array, o = ["OBJECT", "EMBED", "VIDEO"], i = o.length, n = [], m = 0; i > m; m++) {
      n = void 0 != arguments[0] ? e.getElementsByTagName(o[m]) : document.getElementsByTagName(o[m]);
      for (var a = 0; a < n.length; a++) {
        var s = getTargetVideo(n[a]);
        s && t.push(s)
      }
    }
    return t
  }

  function getTargetVideo(el) {
    if (el.nodeType != 1)
      return false;
    if ("VIDEO" == el.tagName)
      return el;
    if (el.type == "application/x-shockwave-flash") {
      return el;
    }
    if ("OBJECT" == el.tagName || "EMBED" == el.tagName) {
      if ((el.querySelectorAll("param[name='type']").length > 0) &&
          (el.querySelectorAll("param[name='type']")[0].getAttribute('value') == "application/x-shockwave-flash")) {
        return el;
      }
    }
    return false;
  }

  function addMouseListener(e, t, o) {
    if (null != e && "" != e && "string" != typeof e && ("function" != typeof e || "undefined" != typeof e.nodeName)) {
      var i = "on" + t;
      e.addEventListener ? e.addEventListener(t, o, !1) : e.attachEvent ? e.attachEvent(i, o) : e[i] = o
    }
  }

  function removeMouseListener(e, t, o) {
    if (null != e && "" != e && "string" != typeof e && ("function" != typeof e || "undefined" != typeof e.nodeName)) {
      var i = "on" + t;
      e.removeEventListener ? e.removeEventListener(t, o, !1) : e.detachEvent ? e.detachEvent(i, o) : e[i] = null
    }
  }

  function hideHtmlAndBody() {
    for (var e = [document.documentElement, document.body], t = 0; t < e.length; t++) {
      var o = e[t];
      o.style.overflow = "hidden"
    }
  }

  function showHtmlAndBody() {
    for (var e = [document.documentElement, document.body], t = 0; t < e.length; t++) {
      var o = e[t];
      o.style.overflow = "";
      (null == o.getAttribute("style") || "" == o.getAttribute("style")) && o.removeAttribute("style")
    }
  }

  function setAttribute(e, t, o) {
    e.setAttribute(t, o)
  }

  function removeAttribute(e, t) {
    e.removeAttribute(t)
  }

  function hideDisplayNone() {
    for (var e = document.querySelectorAll("[nfs_display=none]"), t = 0; t < e.length; t++) {
      var o = window.getComputedStyle(e[t], null);
      //"block" == o.getPropertyValue("display") && (e[t].style.visibility = "hidden")
      //zhangyq
      "block" == o.getPropertyValue("display") && (e[t].style.visibility = "hidden") && (e[t].style.display = "none");
    }
  }

  function toolBar() {
    this.box = document.getElementById("nfs-toolbar"),
    this.pos = {
      x: 0,
      y: 0
    }
  }

  function showToolBar(e) {
    gs.toolBar.box.style.display = "block", gs.toolBar.box.style.visibility = "visible", gs.toolBar.box = document.getElementById("nfs-toolbar"), gs.toolBar.box.style.top = (e.box_top + document.body.scrollTop) + "px", void(gs.toolBar.box.style.left = e.box_left + "px"),
    gs.showStatus = 1
  }

  function hideToolBar() {
    window.removeEventListener("scroll", gs.scrollPostion, !1);
    if (gs.toolBar.box) {
      gs.toolBar.box.style.display = "none",
      gs.toolBar.box.style.visibility = "hidden"
    }
  }

  function cWidth() {
    var e,
    t,
    o = document.createElement("DIV");
    return o.style.cssText = "position:absolute; top:-1000px; width:100px; height:100px; overflow:hidden;",
    e = document.body.appendChild(o).clientWidth,
    o.style.overflowY = "scroll",
    t = o.clientWidth,
    document.body.removeChild(o),
    e - t
  }

  function cHeight() {
    var e,
    t,
    o = document.createElement("DIV");
    return o.style.cssText = "position:absolute; top:-1000px; width:100px; height:100px; overflow:hidden;",
    e = document.body.appendChild(o).clientHeight,
    o.style.overflowX = "scroll",
    t = o.clientHeight,
    document.body.removeChild(o),
    e - t
  }

  function scrollCo(e) {
    for (var t = e ? [e] : [document.documentElement, document.body], o = !1, i = !1, n = 0; n < t.length; n++) {
      var r = t[n],
      a = r.scrollLeft;
      r.scrollLeft += a > 0 ? -1 : 1,
      r.scrollLeft !== a && (o = o || !0),
      r.scrollLeft = a;
      var s = r.scrollTop;
      r.scrollTop += s > 0 ? -1 : 1,
      r.scrollTop !== s && (i = i || !0),
      r.scrollTop = s
    }
    return {
      scrollX: o,
      scrollY: i
    }
  }

  function parseMouseEvent(e, t) {
    if ("mouseout" != e.type && "mouseover" != e.type)
      return !1;
    for (var o = e.relatedTarget ? e.relatedTarget : "mouseout" == e.type ? e.toElement : e.fromElement; o && o != t; )
      o = o.parentNode;
    return o != t
  }

  function getShowPosition() {
    if (void 0 == typeof gs.targetVideo)
      return void d("Can't find flash");
    if (gs.targetVideo.offsetWidth <= 360 && gs.targetVideo.offsetHeight <= 240)
      return !1;
    if (location.hostname == "www.iqiyi.com" &&  gs.targetVideo.offsetWidth <= 426 && gs.targetVideo.offsetHeight <= 240)
      return !1;
    if (gs.targetVideo.offsetWidth >= 1400 && gs.targetVideo.offsetWidth / gs.targetVideo.offsetHeight > 1.95)
      return !1;

    var e = gs.targetVideo,
    t = e.getBoundingClientRect().top + document.documentElement.scrollTop,
    o = e.getBoundingClientRect().left + document.documentElement.scrollLeft,
    i = e.offsetWidth,
    n = e.offsetHeight;
    if (26 > t + n)
      return void hideToolBar();
    var a = 124,
    s = 26,
    l = {
      x: 0,
      y: 0
    };
    l.y = t,
    l.x = o + i - a + document.body.scrollLeft;
    var c = 0,
    p = 0,
    u = scrollCo();
    u.scrollY && (c = cWidth()),
    u.scrollX && (p = cHeight());
    var g = {
      box_top: l.y,
      box_left: l.x,
      box_width: a,
      box_height: s,
      scroll_h: document.body.scrollLeft,
      scroll_v: document.body.scrollTop,
      div_pos_x: o,
      div_pos_y: t,
      div_width: i,
      div_height: n,
      hscroll_size: c,
      vscroll_size: p
    };
    window.addEventListener("scroll", scrollPostion, !1);
    //var f = JSON.stringify(g);
    showToolBar(g);
  }

  function scrollPostion() {
    var scrollT = (document.documentElement && document.body) ? document.body.scrollTop : document.documentElement.scrollTop;
    var e = gs.toolBar.box,
    t = gs.targetVideo.getBoundingClientRect().top;
    t > 26 ? e.style.top = (scrollT + t - 26) + "px" : e.style.top = scrollT + "px";
  }

  function createStyle() {
    var e = "#nfs-toolbar,#nfs-toolbar *,#nfs-toolbar :after,#nfs-toolbar :before{box-sizing:initial!important}#nfs-toolbar{position:absolute;left:0;top:10px;z-index:99999;width:123px;height:26px;font-size:14px;background-color:#fff;overflow:hidden}#nfs-toolbar a{position:absolute;top:0;line-height:24px;text-align:center}#nfs-toolbar-play{left:0;width:98px;height:24px;border:1px solid #aaa;font-size:14px;color:#555;font-family:'Microsoft YaHei';text-decoration:none}a#nfs-toolbar-play:hover{background-color:#4380d0;color:#fff}a#nfs-toolbar-play span{display:inline-block;width:16px;height:16px;background:-webkit-image-set(url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAG1BMVEUAAABDgNBDgNBDgNBDgNBDgNBDgNBDgNBDgNDDKOpmAAAACHRSTlMAjOa1JAxiYatfNDoAAAAuSURBVAjXY4CDDijAwmiAqOAAMUxgjA5RGKOpAMroaISJJMDUoOtiQGEgrIABAEEDHGinN93XAAAAAElFTkSuQmCC') 1x);margin-right:4px;overflow:hidden;vertical-align:middle}a#nfs-toolbar-play:hover span{background:-webkit-image-set(url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAHlBMVEUAAAD///////////////////////////////////8kfJuVAAAACXRSTlMAjPnmtSQMYmHVGJIjAAAAMElEQVQI12OAg5lQgIUxAaKCE8RwhTEmiUEZMyc3QBkzJ8JECmBq0HUxoDAQVsAAAGvoIAD4XDxUAAAAAElFTkSuQmCC') 1x)}a#nfs-toolbar-play:active span{background:-webkit-image-set(url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQBAMAAADt3eJSAAAAHlBMVEUAAAD///////////////////////////////////8kfJuVAAAACXRSTlMAjPnmtSQMYmHVGJIjAAAAMElEQVQI12OAg5lQgIUxAaKCE8RwhTEmiUEZMyc3QBkzJ8JECmBq0HUxoDAQVsAAAGvoIAD4XDxUAAAAAElFTkSuQmCC') 1x)}#nfs-toolbar-close{right:0;width:22px;height:24px;border:1px solid #aaa}#nfs-toolbar-close{background:-webkit-image-set(url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOAQMAAAAlhr+SAAAABlBMVEUAAABDgNBq68DJAAAAAXRSTlMAQObYZgAAACZJREFUCNdjAIIECQYDAwaJBAaeAwzsDQzMDEASyAaKAMVBsmAAAHXpBUrCtzBMAAAAAElFTkSuQmCC') 1x) no-repeat;background-position:center center}#nfs-toolbar-close:hover{background:-webkit-image-set(url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOAQMAAAAlhr+SAAAABlBMVEUAAAD///+l2Z/dAAAAAXRSTlMAQObYZgAAACZJREFUCNdjAIIECQYDAwaJBAaeAwzsDQzMDEASyAaKAMVBsmAAAHXpBUrCtzBMAAAAAElFTkSuQmCC') 1x) #4380d0 no-repeat;background-position:center center}#nfs-toolbar-close:active{background:-webkit-image-set(url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAA4AAAAOAQMAAAAlhr+SAAAABlBMVEUAAAD///+l2Z/dAAAAAXRSTlMAQObYZgAAACZJREFUCNdjAIIECQYDAwaJBAaeAwzsDQzMDEASyAaKAMVBsmAAAHXpBUrCtzBMAAAAAElFTkSuQmCC') 1x) #2b67b5 no-repeat;background-position:center center}",
    t = document.getElementsByTagName("head")[0];
    void 0 == t && (t = document.getElementsByTagName("html")[0]);
    var o = document.createElement("style");
    o.type = "text/css",
    o.styleSheet ? o.styleSheet.cssText = e : o.appendChild(document.createTextNode(e)),
    t.appendChild(o)
  }

  function createSpxcHtml() {
    if (void 0 == document.getElementById("nfs-toolbar")) {
      createStyle();
      var e = window.document.createElement("div");
      e.id = "nfs-toolbar",
      e.style.display = "none",
      e.innerHTML = '<a href="javascript:;" target="_self" id="nfs-toolbar-play" ondragstart="javascript:return false;" oncontextmenu="return false"><span></span>小窗口播放</a><a href="javascript:;" target="_self" id="nfs-toolbar-close" oncontextmenu="return false" ondragstart="javascript:return false;"></a>',
      document.body.appendChild(e),
      gs.toolBar.box = document.getElementById("nfs-toolbar")
    } else {
      if (!gs.showStatus)
        hideToolBar()
    }
  }

  function run() {
    if (window.top == window.self) {
      clearTimeout(gs.runTimer);
      var t = getTarget();
      if (t.length) {
        unbindListen();
        createSpxcHtml();
        t.push(gs.toolBar.box);
        gs.mouseHover = setMouseHover(t),
        gs.mouseHover.hover();
        addMouseListener(document.getElementById("nfs-toolbar-play"), "click", openSpxc),
        addMouseListener(document.getElementById("nfs-toolbar-close"), "click", closeSpxc)
      }

      var mess = {type:"NFS_VIDEO_IS_POPUP_WINDOW"};
      chrome.runtime.sendMessage(mess, function isVideoPopupWindow(o) {if (o) { gs.showStatus = 1; if (gs.isVideoPopupWindow++) {autoDisplay();}}});

      if (!gs.showStatus) {
        gs.runTimer = setTimeout("run()", 500)
      }
    }
  }

  function autoDisplay() {
    var mess = {type:"NFS_VIDEO_GET_VIDEO_POPUP_WINDOW_INFO"};
    chrome.runtime.sendMessage(mess, function getVideoPopupWindowInfo(o) {
      var t = o.info.split("|");
      if ("" != t[0] || "" != t[1]) {
        if ("" != t[1]) {
          var o = "#" + t[1];
          gs.currentTarget = document.querySelector(o)
        } else if ("" != t[0]) {
          var i = "." + t[0];
          gs.currentTarget = document.querySelector(i)
        }
        if (null == gs.currentTarget) {
          var n = getTarget();
          if ("" == n)
            return;
          gs.targetVideo = n[0],
          gs.currentTarget = gs.targetVideo.parentNode
        } else {
          var a = getTarget(gs.currentTarget);
          gs.targetVideo = a[0]
        }
        openSpxc()
      }
    });
  }

  function setMouseHover(e) {
    var e = e,
    t = 0,
    o = function () {
      var e = gs.targetVideo;
      e instanceof Array || (removeMouseListener(e, "mouseover", mouseOnover), removeMouseListener(e, "mouseout", mouseOnout))
    },
    i = function () {
      for (t in e) {
        removeMouseListener(e[t], "mouseover", mouseOnover), removeMouseListener(e[t], "mouseout", mouseOnout);
        //work around for youku video tag can't receive mouse event.
        if (location.hostname == "v.youku.com" ||
            location.hostname.includes("tudou.com") ||
            ((navigator.platform =="Win32" || navigator.platform =="Windows") && location.hostname =="v.qq.com")) {
          var node = e[t].nodeName.toLowerCase();
          if (node == 'video' && e[t].parentElement) {
            removeMouseListener(e[t].parentElement, "mouseover", mouseOnover), removeMouseListener(e[t].parentElement, "mouseout", mouseOnout);
          }
        }
      }
    },
    n = function () {
      for (t in e) {
        addMouseListener(e[t], "mouseover", mouseOnover), addMouseListener(e[t], "mouseout", mouseOnout);
        //work around for youku video tag can't receive mouse event.
        if (location.hostname == "v.youku.com" ||
            location.hostname.includes("tudou.com") ||
            ((navigator.platform =="Win32" || navigator.platform =="Windows") && location.hostname =="v.qq.com")) {
          var node = e[t].nodeName.toLowerCase();
          if (node == 'video' && e[t].parentElement) {
            addMouseListener(e[t].parentElement, "mouseover", mouseOnover), addMouseListener(e[t].parentElement, "mouseout", mouseOnout);
          }
        }
      }
    };
    return this.prototype = {
      hover: n,
      unhover: o,
      unhoverall: i
    }
  }

  function mouseOnout(e) {
    clearTimeout(gs.timer);
    var e = e || window.event;
    gs.timer = setTimeout(function () {
          parseMouseEvent(e, this) && hideToolBar(e)
        }, 300)
  }

  function mouseOnover(e) {
    clearTimeout(gs.timer);
    var e = e || window.event;
    if (parseMouseEvent(e, this)) {
      var t = e.target.nodeName.toLowerCase();
      ("embed" == t || "object" == t || "video" == t) && (gs.targetVideo = e.target, gs.currentTarget = e.target.parentNode, "object" == gs.currentTarget.nodeName.toLowerCase() && (gs.currentTarget = gs.currentTarget.parentNode));
      if (location.hostname == "v.youku.com" ||
          location.hostname.includes("tudou.com") ||
          ((navigator.platform == "Win32" || navigator.platform == "Windows") && location.hostname == "v.qq.com")) {
        var v = e.currentTarget.querySelector('video');
        v && (gs.targetVideo = v, gs.currentTarget = v.parentNode);
      }
      chrome.accessPreference.GetBoolean("browser.videoToolbar", function(enabled) {
        if (enabled) {
          getShowPosition();
        }
      });
    }
  }

  function openSpxc() {
    clearInterval(gs.videoProcessTimer);
    if (!gs.isVideoPopupWindow) {
      var mess = {type:"NFS_VIDEO_POPUP_WINDOW"};
      chrome.runtime.sendMessage(mess);
    }
    var e = gs.targetVideo.getAttribute("style"),
    t = gs.currentTarget.getAttribute("style"),
    o = gs.targetVideo.getAttribute("height"),
    i = gs.currentTarget.className + "|" + gs.currentTarget.id + "|0|" + e + "|" + t + "|" + document.body.scrollTop + "|" + o;
    // var videoPopupWindowInfo = {type:"NFS_VIDEO_SET_VIDEO_POPUP_WINDOW_INFO", videoPopupWindowInfo: i};
    // chrome.runtime.sendMessage(videoPopupWindowInfo);
    //zhangyq
    if (!gs.currentTarget.hasAttribute('nfs_style')) {
      var videoPopupWindowInfo = {type:"NFS_VIDEO_SET_VIDEO_POPUP_WINDOW_INFO", videoPopupWindowInfo: i};
      chrome.runtime.sendMessage(videoPopupWindowInfo);
    }
    hideProcess(),
    unbindListen(!0),
    gs.videoProcessTimer = window.setInterval("ProcessVideo()", 350);
  }

  function closeSpxc() {
    unbindListen(!0)
  }

  function unbindListen(e) {
    e && hideToolBar(),
    void 0 != gs.mouseHover && gs.mouseHover.unhover(),
    void 0 != gs.mouseHover && (gs.mouseHover.unhoverall(), removeMouseListener(document.getElementById("nfs-toolbar-close"), "click", closeSpxc), removeMouseListener(document.getElementById("nfs-toolbar-play"), "click", openSpxc))
  }

  function ProcessVideo() {
    hideDisplayNone(),
    hideHtmlAndBody();

    switch ("100%" != gs.targetVideo.style.width && ("function" == typeof gs.targetVideo && (gs.targetVideo.style.width = "100%"), gs.targetVideo.style.width = "100%"), "100%" != gs.targetVideo.style.height && ("function" == typeof gs.targetVideo && (gs.targetVideo.style.height = "100%"), gs.targetVideo.style.height = "100%"), location.hostname) {
    case "www.iqiyi.com":
      if ("100% !important" != gs.currentTarget.style.width) {
        var n = "position:fixed;padding:0;top:0px;left:0px;width:100% !important;height:100% !important;margin:0px;background:#000;z-index:2147483647 !important; border:0px";
        gs.currentTarget.setAttribute("style", n)
      }
      break;
    case "tv.sohu.com":
      if ("100% !important" != gs.currentTarget.style.width && "100% !important" != gs.currentTarget.style.height) {
        var n = "position:fixed;padding:0;top:0px;left:0px;width:100% !important;height:100% !important;margin:0px;background:#000;z-index:2147483647 !important; border:0px";
        gs.currentTarget.setAttribute("style", n)
      }
      break;
    case "v.pptv.com":
      if ("fixed" != gs.currentTarget.style.position) {
          var n = "position:fixed;padding:0;top:0px;left:0px;width:100% !important;height:100% !important;margin:0px;background:#000;z-index:2147483647 !important; border:0px";
          gs.currentTarget.setAttribute("style", n);
      }
      break;
    case "v.ifeng.com":
      if ("absolute" != gs.currentTarget.style.position) {
          var n = "position:absolute;padding:0;top:0px;left:0px;width:100% !important;height:100% !important;margin:0px;background:#000;z-index:2147483647 !important; border:0px";
          gs.currentTarget.setAttribute("style", n);
      }
      break;
    default:
      if ("100%" != gs.currentTarget.style.width && "100%" != gs.currentTarget.style.height) {
        var a = gs.currentTarget.getAttribute("style");
        gs.currentTarget.setAttribute("style", a + ";width:100% !important; height:100% !important;")
      }
    }
  }

  function hideProcess() {
    hideHtmlAndBody();
    var t = gs.targetVideo, c = gs.currentTarget, a = parseIframe(gs.currentTarget);
    (saveOriStyle(t), setAttribute(t, "nfs_display", "block")),
    (saveOriStyle(c), c.style.width = "100%", c.style.height = "100%", setAttribute(c, "nfs_display", "block")),
    addDisplayNone(a),
    hideBodyChildren(a);
    var s = "position:fixed;padding:0;margin:0px;top:0px;left:0px;width:100% !important;height:100% !important;margin:0px;background:#000;z-index:2147483647 !important; border:0px;";
    gs.currentTarget.setAttribute("style", s);
    var l = gs.targetVideo.getAttribute("style"),
    d = window.getComputedStyle(gs.targetVideo, "null");
    null == l && (l = "");
    var c = d.getPropertyValue("position");
    "" != c && (l += ";position:static;"),
    gs.targetVideo.setAttribute("style", l + "padding:0;margin:0px;width:100% !important;height:100% !important;visibility:visible;")
  }

  function showProcess() {
    for (var i = document.querySelectorAll("[nfs_display=none]"), o = 0; o < i.length; o++)
      i[o].style.display = "", i[o].style.visibility = "", removeAttribute(i[o], "nfs_display"), removeTmpStyle(i[o]);
    for (var n = document.querySelectorAll("[nfs_display=block]"), o = 0; o < n.length; o++)
      n[o].style.padding = "", n[o].style.position = "", n[o].style.transform = "", n[o].style.overflow = "", removeAttribute(n[o], "nfs_display"), removeTmpStyle(n[o]);
    showHtmlAndBody()
  }

  chrome.runtime.onMessage.addListener(
    function(request, sender, sendResponse) {
      if (!request.isVideoPopupWindow) {
        clearInterval(gs.videoProcessTimer);
        gs.isVideoPopupWindow = 0;
        gs.showStatus = 0;
        showProcess();
        run();
      }
  });

  var gs = new globalStruct;
  gs.toolBar = new toolBar;
  run();
