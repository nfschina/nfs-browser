/**
 * Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, CAS
 *
 * This software is published by the license of CPU-OS Licence, you can use and
 * distribute this software under this License. See CPU-OS License for more detail.
 *
 * You should have received a copy of CPU-OS License. If not, please contact us
 * by email <support_os@cpu-os.ac.cn>
 *
**/

cr.define('incognito', function() {
  'use strict'

  function onDefaultSearchEngineChanged(data) {
    $('#engine-logo').css('background-image', 'url(' + data.favicon + ')');
  }

  // 处理输入联想事件
  function handleKey(keys) {
    var keywordBox = $('#keyword');
    var keyWord = $('#keyword');
    var len = keys.s.length;

    if (len == 0) {
      keyWord.css('display', 'none');
    } else {
      keyWord.css('display', 'block');
    }

    var spans = "";
    for (var i = 0; i < len; ++i) {
      spans += '<span>' + keys.s[i] + "</span>";
    }

    // 将关键词写入联想框
    keyWord.html(spans);
    keyWord.animate({
      height: (keyWord.children().height()) * len
    }, 100);

    // 点击候选词汇
    keyWord.children().click(function() {
      $('#input').value = $(this).html();

      keyWord.animate({
        height: 0
      }, 10, function() {
        keyWord.css('display', 'none');
        keyWord.css('height', 'auto');
        keyWord.html('');
      });

      $('#search-bt').click();
    });

    // 鼠标离开联想框后收缩联想框
    keyWord.mouseleave(function() {
      keyWord.animate({
        height: 0
      }, 100, function() {
        keyWord.css('display', 'none');
        keyWord.css('height', 'auto');
        keyWord.html('');
      });
    });

    var numspan = 0;
    $('#input').on('keydown', function(e) {
      // 如果使用回车键提交，联想框也可以自动收缩
      if (e.which == 13) {
        keyWord.animate({
          height: 0
        }, 10, function() {
          keyWord.css('display', 'none');
          keyWord.css('height', 'auto');
          keyWord.html('');
        });
      }

      // 按下下方向键
      if (e.which == 40) {
        if (numspan == len) {
          numspan = 0;
        }

        for (var i = 0; i < len; i++) {
          if (numspan == i) {
            keyWord.children().eq(i).css('background-color', '#deecff');
          } else {
            keyWord.children().eq(i).css('background-color', 'rgba(255, 255, 255, 0.3)');
          }
        }

        $('#input').val(keyWord.children().eq(numspan).html());
      }

      // 按下上方向键
      if (e.which == 38) {
        if (numspan == 0) {
          numspan = len;
        }
        numspan--;

        for (var i = 0; i < len; i++) {
          if (numspan == i) {
            keyWord.children().eq(i).css('background-color', '#deecff');
          } else {
            keyWord.children().eq(i).css('background-color', 'rgba(255, 255, 255, 0.3)');
          }
        }

        $('#input').val(keyWord.children().eq(numspan).html());
      }
    });

    keyWord.children().mouseover(function() {
      numspan = $(this).index();
      for (var i = 0; i < len; i++) {
        if (numspan == i) {
          keyWord.children().eq(i).css('background-color', '#deecff');
        } else {
          keyWord.children().eq(i).css('background-color', 'rgba(255, 255, 255, 0.3)');
        }
      }
      $('#input').val(keyWord.children().eq(numspan).html());
    });
  }

  // 绘制搜索框
  function renderSearchBox(searchEngine) {
    var engineLogo = $('<a>').attr('id', 'engine-logo');
    engineLogo.css('background-image', 'url(' + searchEngine.favicon + ')');
    engineLogo.append($('<span>'));
    engineLogo.on('click', function() {
      chrome.send('getSearchEngines', []);
    });

    var searchEngine  = $('<div>').attr('id', 'search-engine');
    searchEngine.prepend(engineLogo);

    var searchInput   = $('<div>').attr('id', 'search-input');
    var inputHtml = '<input id="input" type="text">'
                  + '<span id="search-bt"></span>';
    searchInput.html(inputHtml);

    $('#search-box').append(searchEngine, searchInput);

    // 点击搜索图标搜索
    $('#search-bt').on('click', function() {
      var key = $('#input').val();
      chrome.send('startSearch', [key]);
    });

    // 回车键搜索
    $('#input').on('keydown', function(e) {
      if (e.keyCode == 13) {
        var key = $(this).val();
        chrome.send('startSearch', [key]);
      }
    });

    // 输入联想事件响应
    $('#input').on('keyup', function(e) {
      if ($(this).val() === "" || $(this).val() === " ")
        return ;

      if (e.which != 39 && e.which != 40 && e.which != 37 && e.which != 38 && e.which != 13) {
        $.ajax({
          url: "http://suggestion.baidu.com/su",
          type: "GET",
          dataType: "jsonp",
          jsonp: "jsoncallback",
          async: false,
          timeout: 5000, //请求超时
          data: {
            "wd": input.value,
            "cb": "ntp.handleKey"
          },
          success: function(json) {
            console.log(json);
          },
          error: function(xhr) {
            return ;
          }
        });
      }
    });
  }


  // 绘制搜索引擎图标
  function renderEngineLogo() {
    var url = $(this).attr('url');
    var favicon = $(this).attr('favicon');
    var name = $(this).text();
    var index = $(this).attr('index');

    $('#engine-logo').css('background-image', 'url(' + favicon + ')');

    // 修改搜索引擎
    chrome.send('setSearchEngine', [parseInt(index)]);
  }

  // 获取当前默认的搜索引擎
  function getDefaultSearchEngine(defaultSearchEngine) {
    renderSearchBox(defaultSearchEngine);
  }

  // 获取所有搜索引擎
  function getSearchEngines(data) {
    var searchContent = $('#search-content');
    var engineLogo = $('#engine-logo');

    searchContent.css('display', 'block');
    searchContent.html("");

    var ul = $('<ul>')
    for (var i = 0; i < data.length; ++i) {
      var li = $('<li>').css('background-image', 'url(' + data[i].favicon + ')')
                        .attr('title', data[i].name)
                        .attr('favicon', data[i].favicon)
                        .attr('url', data[i].url)
                        .attr('index', data[i].index)
                        .text(data[i].name)
                        .on('click', renderEngineLogo);
      ul.append(li);
    }
    searchContent.append(ul);

    $(document).on('click', function() {
      $('#search-content').css('display', 'none');
      $(document).off('click');
    })
  }

  function onLoad() {
    setFavicon(loadTimeData.getString("is_icons_dark"));
    chrome.send('getDefaultSearchEngine', []);
  }

  function setFavicon(is_icons_dark) {
    var favcion_url = null;
    if (is_icons_dark == "true") {
      favcion_url = 'nfsbrowser://theme/IDD_PRIVATEWINDOWICON_N_DARK';
    } else {
      favcion_url = 'nfsbrowser://theme/IDD_PRIVATEWINDOWICON_N';
    }
    var fav_link =document.getElementById("favicon");
    fav_link.setAttribute("href", favcion_url);
  }

  function themeChanged(data) {
    setFavicon(data.is_icons_dark);
  }

  return {
    getDefaultSearchEngine: getDefaultSearchEngine,
    getSearchEngines: getSearchEngines,
    handleKey: handleKey,
    onDefaultSearchEngineChanged: onDefaultSearchEngineChanged,
    onLoad: onLoad,
    setFavicon: setFavicon,
    themeChanged: themeChanged
  }
});

document.addEventListener('DOMContentLoaded', incognito.onLoad);
