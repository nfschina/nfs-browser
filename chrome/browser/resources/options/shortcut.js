
// var $$ = jQuery.noConflict();

cr.define('options', function() {
  var OptionsPage = options.OptionsPage;
  var Page = cr.ui.pageManager.Page;


  function ShortCut() {
    Page.call(this, 'shortcut', '\u9009\u9879 - \u5feb\u6377\u952e', 'shortcutPage');
  }

  cr.addSingletonGetter(ShortCut);

  function findAncestorWithClass(node, className) {
    do {
      if (node.classList && node.classList.contains(className)) return node;
    } while (node = node.parentNode);
    return null;
  }

  function findAncestorWithTagName(node, tagName) {
    do {
      if (node.classList && node.tagName.toUpperCase() == tagName.toUpperCase()) return node;
    } while (node = node.parentNode);
    return null;
  }

  function merge(toObj, fromObj, force) {
    for (var k in fromObj) {
      if (!(k in toObj) || force) {
        toObj[k] = fromObj[k];
      }
    }
  }





  window.showTip = function(msg, isErr) {

    var fTipDom = document.querySelector('.savemsg');
    var  tipWrap = fTipDom.parentNode;
    if (fTipDom) {
      tipWrap.removeChild(fTipDom);
    }
    var tipDom = document.createElement('div');
    tipDom.className = 'savemsg trans-hide';
    tipDom.addEventListener('webkitTransitionEnd', function() {
      if (!tipDom.classList.contains('trans-hide')) {
        window.setTimeout(function() {
          tipDom.classList.add('trans-hide');
        }, 3000);
      }
    });

    tipDom.innerHTML = msg;
    if (isErr) {
      tipDom.style.backgroundColor = '#f93';
    }
    tipWrap.insertBefore(tipDom, tipWrap.firstChild);
    tipDom.style.marginLeft = (-tipDom.getBoundingClientRect().width / 2) + 'px';
    window.setTimeout(function() {
      tipDom.classList.remove('trans-hide');
    });
  
  };


   var ctlKey = {
    16: 'Shift',
    17: 'Ctrl',
    18: 'Alt'
  };

  var endKey = {
    8: 'Backspace',
    9: 'Tab',
    20: 'Caps Lock',
    27: 'Esc',
    33: 'PgUp',
    34: 'PgDown',
    35: 'End',
    36: 'Home',
    37: '←',
    38: '↑',
    39: '→',
    40: '↓',
    45: 'Insert',
    46: 'Delete',
    48: '0',
    49: '1',
    50: '2',
    51: '3',
    52: '4',
    53: '5',
    54: '6',
    55: '7',
    56: '8',
    57: '9',
    65: 'A',
    66: 'B',
    67: 'C',
    68: 'D',
    69: 'E',
    70: 'F',
    71: 'G',
    72: 'H',
    73: 'I',
    74: 'J',
    75: 'K',
    76: 'L',
    77: 'M',
    78: 'N',
    79: 'O',
    80: 'P',
    81: 'Q',
    82: 'R',
    83: 'S',
    84: 'T',
    85: 'U',
    86: 'V',
    87: 'W',
    88: 'X',
    89: 'Y',
    90: 'Z',
    96: 'Num0',
    97: 'Num1',
    98: 'Num2',
    99: 'Num3',
    100: 'Num4',
    101: 'Num5',
    102: 'Num6',
    103: 'Num7',
    104: 'Num8',
    105: 'Num9',
    107: 'Num+',
    109: 'Num-',
    112: 'F1',
    113: 'F2',
    114: 'F3',
    115: 'F4',
    116: 'F5',
    117: 'F6',
    118: 'F7',
    119: 'F8',
    120: 'F9',
    121: 'F10',
    122: 'F11',
    123: 'F12',
    144: 'Num Lock',
    145: 'Scroll Lock',
    186: ';',
    187: '=',
    188: ',',
    189: '-',
    190: '.',
 //   191: '/',
    192: '`',
    219: '[',
    220: '\\',
    221: ']',
    222: '\''
  };

  var sysShortcuts = [
    ['Ctrl', 'Shift', 78],
    ['Ctrl', 187],
    ['Ctrl', 189],
    ['Ctrl', 48],
    ['Ctrl', 80],
    ['Ctrl', 'Shift', 46]
  ];

  var osShortcuts = [
    ['Ctrl', 88],
    ['Ctrl', 67],
    ['Ctrl', 86],
    ['Ctrl', 65],
    ['Ctrl', 90]
  ];
  /*var osShortcuts = [
    ['Ctrl', 'C'],
    ['Ctrl', 'V'],
    ['Ctrl', 'Z'],
    ['Ctrl', 'X'],
    ['Ctrl', 'A']
  ]*/

  var idWordMap = {
    34014: "open_tab",
    34027: "copy_tab",
    33003: "open_homepage",
    34015: "close_tab",
    34050: "close_all_tab",
    34028: "reopen_the_last_tab",
    34017: "switch_to_previous_tab",
    34016: "switch_to_next_tab",
    34000: "open_new_window",
    39001: "select_address_bar",
    34001: "open_new_stealth_window",
   20018: "dropdown_list",
    33006: "stop",
    33002: "refresh",
    33007: "force_refresh",
    // 33007: "强制刷新当前页面",
    38001: "page_zoom",
    38003: "page_shrink",
    38002: "restore_page",
    34030: "full_screen",
    35004: "save_current_page",
    39009: "save_pgae_as_picture",
    35003: "print",
    40000: "open_file",
    37000: "page_search",
    40012: "open_downloadmanager",
    40010: "view_history",
    40013: "clear_traces",
    35000: "add_bookmark",
    40009: "show_hide_bookmarkbar",
 //  20019: "显示/隐藏侧边栏",
    // 20020: "浏览器静音",
    // 40019: "浏览器医生",
    40003: "developer_tool",
    // 40251: "开启/关闭图片放大镜"
  };

  var categoryMap = {
    34014: "tab",
    34027: "tab",
    33003: "tab",
    34015: "tab",
    34050: "tab",
    34028: "tab",
    34017: "tab",
    34016: "tab",
    34000: "tab",
    34001: "tab",
    39001: "browse",
    20018: "browse",
    33006: "browse",
    33002: "browse",
    33007: "browse",
    // 33007: "browse",
    38001: "browse",
    38003: "browse",
    38002: "browse",
    34030: "browse",
    35004: "tool",
    39009: "tool",
    35003: "tool",
    40000: "tool",
    37000: "tool",
    40012: "tool",
    40010: "tool",
    40013: "tool",
    35000: "tool",
    40009: "tool",
   20019: "tool",
    // 20020: "tool",
    // 40019: "tool",
    40003: "tool",
    40251: "tool"
  };


  function reserveQueryEndKey(keyStr) {
    var rTable = reserveQueryEndKey.reserveTable;
    if (!rTable) {
      rTable = {};
      for (var keyCode in endKey) {
        rTable[endKey[keyCode]] = keyCode;
      }
      reserveQueryEndKey.reserveTable = rTable;
    }

    return rTable[keyStr];
  }

  function renderShortCutsTr(json, actionId) {

    if (!idWordMap[actionId]) {
      return;
    }

    var tr, td, div, span1, span2, a, b;

    tr = document.createElement('tr');
    tr.dataset.actionId = actionId;

    td = document.createElement('td');
    td.innerHTML = idWordMap[actionId];
    tr.appendChild(td);
    td = document.createElement('td');
    for (var k in json) {
      div = document.createElement('div');
      div.className = 'normal';
      span1 = document.createElement('span');
      span1.className = 'shortcuts normal-shortcuts';
      span1.title = loadTimeData.getString('click_to_modify_shortcut_keys');
      for (var n in json[k]['Keys']) {
        if (n != 0) {
          span1.appendChild(document.createTextNode(' + '));
        }
        span2 = document.createElement('span');
        span2.innerHTML = endKey[json[k]['Keys'][n]] || json[k]['Keys'][n];
        span1.appendChild(span2);
      }
      div.appendChild(span1);
      if (isSysShortcuts(parseShortCutsSpan(span1))) {
        div.className = 'sys';
        span1.title = loadTimeData.getString('system_shortcuts_not_editable');
      }

      if (Object.keys(json).length > 1) {
        b = document.createElement('b');
        b.className = 'remove';
        b.title = loadTimeData.getString('delete_shortcut');
        div.appendChild(b);
        span1 = document.createElement('span');
        span1.className = 'shortcuts edit-shortcuts';
        div.appendChild(span1);
      } else {
        span1 = document.createElement('span');
        span1.className = 'shortcuts edit-shortcuts';
        div.appendChild(span1);
      }
      td.appendChild(div);
    }
    if (td.querySelectorAll('.normal').length + td.querySelectorAll('.sys').length < 3) {
      a = document.createElement('a');
      a.className = 'add';
      td.appendChild(a);
    }
    tr.appendChild(td);
    return tr;
  }

  function parseShortCutsTr(tr) {
    if (!tr) {
      return;
    }
    var json = {},
      shortcuts = [],
      normalSpans = tr.querySelectorAll('span.normal-shortcuts');

    for (var i = 0, normalSpan; normalSpan = normalSpans[i++];) {
      shortcuts.push({
        Keys: parseShortCutsSpan(normalSpan)
      });
    }

    json[tr.dataset.actionId] = shortcuts;
    return json;
  }

  function parseShortCutsSpan(normalSpan) {
    var keys = [],
      keySpans = normalSpan.querySelectorAll('span');

    for (var j = 0, keySpan; keySpan = keySpans[j++];) {
      keys.push(reserveQueryEndKey(keySpan.innerHTML) || keySpan.innerHTML);
    }
    return keys;
  }

  function parseShortCuts(shortcuts) {
    var keys = [],
      arr = shortcuts.split(' + ');

    for (var j = 0, item; item = arr[j++];) {
      keys.push(reserveQueryEndKey(item) || item);
    }
    return keys;
  }

  function renderShortCutsTBody(json) {
    var tbody = document.querySelector('#shortcutPage tbody:nth-child(2)'),
      tr;
    tbody.innerHTML = '';
    for (var j in json) {
      if (tr = renderShortCutsTr(json[j], j)) {
        tbody.appendChild(tr);
      }
    }
  }

  function renderShortCutsTBody2(json) {
    $$('tbody.browse, tbody.tab, tbody.tool').empty();
    for (var j in json) {
      $$('tbody.' + categoryMap[j]).append(renderShortCutsTr2(json[j], j));
    }
  }

  function renderShortCutsTr2(json, actionId) {
    if (!idWordMap[actionId]) {
      return;
    }
    var html = [];
    json.forEach(function(item) {

      var isSys = isSysShortcuts(item.Keys);
      var title = isSys ? loadTimeData.getString('system_shortcuts_not_editable') : (ShortCut.disabled ? '' : loadTimeData.getString('click_to_modify_shortcut_keys'));
      var sys = isSys ? 'sys-shortcut' : '';
      var disabled = (ShortCut.disabled || isSys) ? ' disabled' : '';
      html.push('<tr data-action-id="' + actionId + '">' +
        // '<td id="aaa">' + idWordMap[actionId] + '</td>' +
        '<td>' + loadTimeData.getString(idWordMap[actionId]) + '</td>' +
        '<td><input type="text" class="' + sys + '" title="' + title + '" data-ori="' + formatShortCuts(item.Keys) + '" value="' + formatShortCuts(item.Keys) + '"' + disabled + '></td>' +
        '</tr>');

      // document.getElementById('aaa').innerHTML = loadTimeData.getString('newtab_type_url');
      // $('#aaa').val(loadTimeData.getString('newtab_type_url'));
    });

    return html.join('');
  }

  function formatShortCuts(keys) {
    var html = [];
    for (var n in keys) {
      if (n != 0) {
        html.push(' + ');
      }
      html.push(endKey[keys[n]] || keys[n]);
    }
    return html.join('');
  }


  function getActionIdFromKeys(keys) {
    var datas = window.loadShortCuts.data,
      same = false;
    for (var i in datas) {
      for (var j = 0; j < datas[i].length; j++) {
        if (keys.length == datas[i][j]['Keys'].length) {
          same = true;

          for (var k = 0; k < keys.length; k++) {
            if (datas[i][j]['Keys'][k] != keys[k]) {
              same = false;
            }
          }
          if (same) {
            return i;
          }
        }
      }
    }
  }

  function resort(wrap) {
    var keys = wrap.querySelectorAll('span'),
      ctrlKey, shiftKey, altKey;
    if (keys.length) {
      for (var i = 0, el; el = keys[i++];) {
        switch (el.innerHTML) {
          case 'Ctrl':
            ctrlKey = el;
            break;
          case 'Shift':
            shiftKey = el;
            break;
          case 'Alt':
            altKey = el;
            break;
        }
      }
    }
    var lastEl = wrap.lastElementChild;
    wrap.innerHTML = '';
    if (ctrlKey) {
      wrap.appendChild(ctrlKey);
      wrap.appendChild(document.createTextNode(' + '));
    }
    if (shiftKey) {
      wrap.appendChild(shiftKey);
      wrap.appendChild(document.createTextNode(' + '));
    }
    if (altKey) {
      wrap.appendChild(altKey);
      wrap.appendChild(document.createTextNode(' + '));
    }
    if (lastEl && ['Ctrl', 'Shift', 'Alt'].indexOf(lastEl.innerHTML) == -1) {
      wrap.appendChild(lastEl);
    }
  }

  function resort2(input) {
    var arr = input.value.split(' + ');
    arr.sort(function(x, y) {
      return getWeight(y) - getWeight(x);
    });
    input.value = arr.join(' + ');

    function getWeight(val) {
      switch (val) {
        case 'Ctrl':
          return 9;
        case 'Shift':
          return 8;
        case 'Alt':
          return 7;
        default:
          return 0;
      }
    }
  }

  function isValidate(keys) {
    if (keys && keys.length == 1) return (keys[0] >= 112) && (keys[0] <= 123);
    return keys && keys.length > 1 && endKey[keys[keys.length - 1]] && (keys[0] != 'Shift' || (keys[0] == 'Shift' && (keys[1] < 65 || keys[1] > 90)));
  }

  function isExist(keys) {
    // if (keys && keys.length == 1 && (keys[0] >= 112 && keys[0] <= 123)) {
    //   if ($$('.hotkey-table input[pref$=' + endKey[keys[0]].toLowerCase() + ']').val()) {
    //     return '热键网址';
    //   }
    // }
    var datas = window.loadShortCuts.data,
      same;
    for (var k in datas) {
      for (var j = 0; j < datas[k].length; j++) {
        //console.log(datas[k][j]['Keys'], keys);
        if (datas[k][j]['Keys'].length == keys.length) {
          same = true;
          for (var i = 0; i < keys.length; i++) {
            if (keys[i] != datas[k][j]['Keys'][i]) {
              same = false
              break;
            }
          }
          if (same) {
            return k;
          }
        }
      }
    }
  }

  function isSysShortcuts(keys) {
    if (keys.length == 1) {
      return keys[0] == 27;
    }
    return false;
  }

  function isOsShortcuts(keys) {
    var osKeys,
      same = false;
    for (var i = 0; i < osShortcuts.length; i++) {
      osKeys = osShortcuts[i];
      if (osKeys.length == keys.length) {
        same = true;
        for (var j = 0; j < keys.length; j++) {
          if (keys[j] != osKeys[j]) {
            same = false;
            break;
          }
        }
        if (same == true) {
          return true;
        }
      }
    }
    return false;
  }

  window.saveSuccess = function() {
 //   showShortcutTip('自动保存成功');
    chrome.send('loadShortCuts', ['loadShortCuts', false]);
  };

  function saveShortCuts() {
    var trs = ShortCut.getInstance().pageDiv.querySelectorAll('tr[data-action-id]'),
      json = {};
    for (var i = 0, tr; tr = trs[i++];) {
      var actionId = tr.dataset.actionId;
      var shortcut = {
        Keys: parseShortCuts(tr.querySelector('input').value)
      };
      if (json[actionId]) {
        json[actionId].push(shortcut);
      } else {
        json[actionId] = [shortcut];
      }
      // merge(json, parseShortCutsTr2(tr), true);
    }
    console.log("saveshortcuts"+ json);
    merge(json, window.loadShortCuts.data, false);
    chrome.send('saveShortCuts', ['saveSuccess', JSON.stringify(json)]);
  }

  window.loadShortCuts = function(jsonStr) {
   // window.loadShortCuts.data = JSON.parse(jsonStr);
   window.loadShortCuts.data = jsonStr;
    renderShortCutsTBody2(window.loadShortCuts.data);
  };
  document.addEventListener('DOMContentLoaded', function() {
    window.setTimeout(function() {
      chrome.send('loadShortCuts', ['loadShortCuts', false]);
    });
  });

  window.resetSuccess = function() {
    chrome.send('loadShortCuts', ['loadShortCuts', false]);
   
      // showShortcutTip('恢复默认快捷键!', this);
  };

  // var timer_tips;

  // function showShortcutTip(text, ele) {
  //   $$('.shortcut-table .shortcut-tips').remove();
  //   // $$(ele).nextAll().remove();
  //   $$(ele).after('<div class="shortcut-tips">' + text + '</div>');
  //   var $tips = $$(ele).next();
  //   $tips.on('click', function() {
  //     $tips.remove();
  //     ele.focus();
  //   });
  //   $tips.fadeIn(200, function() {
  //     timer_tips && clearTimeout(timer_tips);
  //     timer_tips = setTimeout(function() {
  //       $tips.fadeOut(200, function() {
  //         $tips.remove();
  //       });
  //     }, 3000);
  //   });
  // }


  var timer_tips;
  function showShortcutTip(text, ele) {
    ele.blur();
    console.log("showShortcutTip"+$$('.shortcut-table .shortcut-tips'));
    if($$('.shortcut-table .shortcut-tips')){
    $$('.shortcut-table .shortcut-tips').remove();
  }
    // $$(ele).nextAll().remove();
    $$(ele).after('<div class="shortcut-tips">' + text + '</div>');
    var $tips = $$(ele).next();
    $tips.on('click', function() {
     $tips.remove();
     ele.focus();
    });
    $tips.fadeIn(200, function() {
      clearTimeout(timer_tips);
      timer_tips = setTimeout(function() {
        $tips.fadeOut(200, function() {
              console.log("showShortcutTip1"+$$('.shortcut-table .shortcut-tips'));
          this.remove();
        });
     ele.focus();
      }, 3000);
    });
  }

  ShortCut.prototype = {
  //  __proto__: options.OptionsPage.prototype,
    __proto__: Page.prototype,

    initializePage: function() {
      Page.prototype.initializePage.call(this);
      document.body.addEventListener('click', this.onClick);

      Preferences.getInstance().addEventListener('browser.enable_short_cuts', function(e) {
        if (e && e.value && e.value.value) {
          ShortCut.disabled = false;
          $$('.shortcut-table').removeClass('disabled');
          $$('.shortcut-table input:not(.sys-shortcut)').attr('disabled', false).attr('title', loadTimeData.getString('click_to_modify_shortcut_keys'));
        } else {
          ShortCut.disabled = true;
          $$('.shortcut-table').addClass('disabled');
          $$('.shortcut-table input:not(.sys-shortcut)').attr('disabled', true).attr('title', '');

        }
      });

      $$(document).delegate('.shortcut-table input', 'keydown', this.onInputKeydown)
        .delegate('.shortcut-table input', 'keyup', this.onInputKeyup)
        .delegate('.shortcut-table input', 'input', function(e) {

          this.value = $$(this).data('ori');
          return false;
        })
        .delegate('.shortcut-table input', 'paste', function() {
          return false;
        })
        .delegate('.shortcut-table input', 'blur', function() {
          this.value = $$(this).data('ori');
        });
    },

  // $$('.defaultKey').onclick = function(event) {
  //          alert("reset click");
  //          chrome.send("resetShortCuts", ['resetShortCuts', false]);
  //     };

    onInputKeydown: function(e) {
      e.preventDefault();
      console.log("input keydown"+e.keyCode);
       if(e.keyCode == 229){
  this.blur();
  showShortcutTip(loadTimeData.getString('not_allow_Chinese'), this);
  
  }
      if (this.lastKeyCode == e.keyCode) {
        return false;
      }
      // chrome.send('enterShortcutEdit', [true]);

      var self = this;

      function clean() {
        if (!self._reset_shortcut) {
          self.value = '';
          self._reset_shortcut = true;
        }
      }

      var key;
      if (key = ctlKey[e.keyCode]) {
        clean();
        var nkey = key + ' + ';
        if (this.value.indexOf(nkey) < 0) {
          this.value += nkey;
        }
        resort2(this);
        this._valid_keydown = true;
      } else if ((key = endKey[e.keyCode]) && ctlKey[this.lastKeyCode]) {
        clean();
        this.value += key;
        this._valid_keydown = true;
      } else if (e.keyCode >= 112 && e.keyCode <= 123) {
        clean();
        this.value += endKey[e.keyCode];
        this._valid_keydown = true;
      }
      this.lastKeyCode = e.keyCode;
    },

    onInputKeyup: function(e) {
      console.log("keyup");
      e.preventDefault();
      console.log("keyup"+e.keyCode);
      if (!this._valid_keydown) {
        return;
      }
      this._valid_keydown = false;
      // chrome.send('enterShortcutEdit', [false]);
      this.lastKeyCode = null;
      this._reset_shortcut = false;
        
      var keys = parseShortCuts(this.value),
        actionId;
      if (isOsShortcuts(keys)) {
        showShortcutTip(loadTimeData.getString('not_allow_system_shortcut'), this);
      } else {
        if (actionId = isExist(keys)) {
          if (idWordMap[actionId]) {
            showShortcutTip(loadTimeData.getString('this_shortcut_already') + loadTimeData.getString(idWordMap[actionId]) + loadTimeData.getString('uesd'), this);
          } else {
            showShortcutTip(loadTimeData.getString('this_shortcut_already_used_by_system'), this);
          }
        } else {
          if (isValidate(keys)) {
            // normalShortcutsEl.innerHTML = editShortcutsEl.innerHTML;
            saveShortCuts();
            return;
          } else {}
        }
      }
      this.value = $$(this).data('ori');
      // this.blur();
    },

    cleanEdit: function() {
      var editEl;
      if (editEl = document.querySelector('.edit') || document.querySelector('.waitedit')) {
        editEl.classList.remove('edit')
        editEl.classList.remove('waitedit')
        editEl.classList.add('normal');
      }
    },
    onClick: function(e) {
      var el;
      console.log("onClick"+ e.target.id);
      if (e.target.id == 'btn-reset-shortcuts') {
      chrome.send('resetShortCuts', ['resetSuccess', false]);
       showTip(loadTimeData.getString('restore_default_success'), true);
        return false;
      }
      if (e.target.classList.contains('remove')) {
        el = findAncestorWithTagName(e.target, 'div');
        el.parentNode.removeChild(el);
        saveShortCuts();
        return false;
      }
      if (e.target.classList.contains('add')) {
        el = findAncestorWithTagName(e.target, 'td');
        var tmp = document.createElement('div');
        tmp.innerHTML = '<div class="normal"><span class="shortcuts normal-shortcuts" id="press_shortcut_key"></span><span class="shortcuts edit-shortcuts"></span><b class="remove"></b></div>';

        el.insertBefore(tmp = tmp.firstChild, e.target);
        window.setTimeout(function() {
          var ev = document.createEvent('HTMLEvents');
          ev.initEvent('click', true, true);
          tmp.dispatchEvent(ev);
        });
        if (el.querySelectorAll('.normal').length > 2) {
          el.querySelector('.add').classList.add('hide');
        } else {
          el.querySelector('.add').classList.remove('hide');
        }

      }

      if (el = findAncestorWithClass(e.target, 'normal')) {
        ShortCut.getInstance().cleanEdit();
        el.classList.remove('normal');
        el.classList.add('waitedit');
        var lastKeyCode,
          normalShortcutsEl = el.querySelector('.normal-shortcuts'),
          editShortcutsEl = el.querySelector('.edit-shortcuts');

        window.onkeydown = function(e) {
           console.log("keyup"+e.keyCode);
          e.preventDefault();
          if (e.keyCode == lastKeyCode) {
            return false;
          }
          // chrome.send('enterShortcutEdit', [true]);
          var key, keyEl, plusEl;
          if (key = ctlKey[e.keyCode]) {
            el.classList.remove('waitedit');
            el.classList.add('edit');
            keyEl = document.createElement('span');
            keyEl.innerHTML = key;
            editShortcutsEl.appendChild(keyEl);
            plusEl = document.createTextNode(' + ');
            editShortcutsEl.appendChild(plusEl);
            resort(editShortcutsEl);
          } else if ((key = endKey[e.keyCode]) && ctlKey[lastKeyCode]) {
            keyEl = document.createElement('span');
            keyEl.innerHTML = key;
            editShortcutsEl.appendChild(keyEl);
          } else if (e.keyCode >= 112 && e.keyCode <= 123) {
            keyEl = document.createElement('span');
            keyEl.innerHTML = endKey[e.keyCode];
            editShortcutsEl.appendChild(keyEl);
          }
          lastKeyCode = e.keyCode;
          return false;
        };

        window.onkeyup = function(e) {
          e.preventDefault();
          ShortCut.getInstance().cleanEdit();
          // chrome.send('enterShortcutEdit', [false]);
          window.onkeydown = null;
          window.onkeyup = null;
          var keys = parseShortCutsSpan(editShortcutsEl),
            actionId;
          if (isOsShortcuts(keys)) {
            showShortcutTip(loadTimeData.getString('not_allow_system_shortcut'), this);
          } else {
            if (actionId = isExist(keys)) {
              if (idWordMap[actionId]) {
                showShortcutTip(loadTimeData.getString('this_shortcut_already') + loadTimeData.getString(idWordMap[actionId]) + loadTimeData.getString('uesd_reset'), this);
              } else {
                showShortcutTip(loadTimeData.getString('this_shortcut_already_used_by_system_reset'), this);
              }
            } else {
              if (isValidate(keys)) {
                normalShortcutsEl.innerHTML = editShortcutsEl.innerHTML;
                saveShortCuts();
              } else {}
            }
          }
          editShortcutsEl.innerHTML = '';
          return false;
        };

      } else {
        if (!findAncestorWithClass(e.target, 'edit') && !findAncestorWithClass(e.target, 'waitedit')) {
          ShortCut.getInstance().cleanEdit();
          window.onkeydown = null;
          window.onkeyup = null;
        }
     }
    }
  };

  ShortCut.getShortCutEndKeys = function() {
    return endKey;
  };

  // ShortCut.updateBosskeyDisplay = function(bosskeys) {
  //   if (bosskeys.length <= 0) return;
  //   var bosskeyStr = '',
  //     len = bosskeys.length - 1;
  //   if (!(len == 0 && bosskeys[0] == 0)) {
  //     for (var i = 0; i < len; i++)
  //       bosskeyStr += bosskeys[i] + '+';
  //     bosskeyStr += ShortCut.getShortCutEndKeys()[bosskeys[len]] || bosskeys[len];
  //   }
  //   $('bosskey_value').textContent = bosskeyStr;
  // };


  return {
    ShortCut: ShortCut
  };
});