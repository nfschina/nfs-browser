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

cr.define('ntp', function() {
  'use strict'

  const pagesCount = 8;
  const defaultThumbnail = 'images/none.png';
  const waitThumbnail = 'images/wait.gif';

  // 判断url是否合法
  function isURL(url){
      return /^(https?|ftp):\/\/(((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:)*@)?(((\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5])\.(\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5])\.(\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5])\.(\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5]))|((([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.)+(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.?)(:\d*)?)(\/((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)+(\/(([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)*)*)?)?(\?((([a-z|]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)|[\uE000-\uF8FF]|\/|\?)*)?(\#((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)|\/|\?)*)?$/i.test(url);
    }

  // 添加网址
  function addWebsite(name, url, index) {
    $('#pages').children().eq(index).empty();
    $('#edit-page').css('display', 'none');
    renderCell(index, url, name, waitThumbnail, 2);

    let websiteList = getCurrentData();
    websiteList[index] = {
      title: name,
      url: url,
      thumbnail: defaultThumbnail
    };
    localStorage.setItem('webList', JSON.stringify(websiteList));

    chrome.send('getThumbnail', [parseInt(index), url]);
  }

  function editPage(type, index) {
    var page = $('#edit-page');
    var webList = getCurrentData();

    page.css('display', 'block');
    page.attr('type', type).attr('index', index);

    var html = "<div id='edit-content'>" +
                 "<div id='edit-cancel'></div>" +
                 "<div id='edit-input'>" +
                   "<input id='edit-name' type='text' />" +
                   "<input id='edit-url' type='text' data-type='url' />" +
                   "<span id='edit-save'></span>" +
                 "</div>" +
                 "<div id='edit-btn'>" +
                   "<span id='edit-default' sld=''></span>" +
                   "<span id='edit-bookmark'></span>" +
                   "<span id='edit-topsite'></span>" +
                 "</div>" +
                 "<div id='edit-display'>" +
                 "</div>" +
               "</div>";
    page.html(html);

    $('#edit-url').attr('placeholder', loadTimeData.getString('newtab_type_url'));
    $('#edit-name').attr('placeholder', loadTimeData.getString('newtab_enter_name'));
    $('#edit-save').text(loadTimeData.getString('add'));
    $('#edit-default').text(loadTimeData.getString('popular_websites'));
    $('#edit-bookmark').text(loadTimeData.getString('bookmarks'));
    $('#edit-topsite').text(loadTimeData.getString('recently_used'));

    $('#edit-url').on('blur', function() {
      // To do ...
    });

    $('#edit-default').on('click', function() {
      $('#edit-display').html('');

      $(this).siblings().removeAttr('sld');
      $(this).attr('sld', '');

      chrome.send('getRecommendedWebsites', []);
    });

    $('#edit-bookmark').on('click', function() {
      $('#edit-display').html('');

      $(this).siblings().removeAttr('sld');
      $(this).attr('sld', '');

      var tree = new cr.ui.Tree();
      tree.id = 'bookmarktree';
      tree.addEventListener('change', function() {
        var selectItem = this.selectedItem;
        if (selectItem && selectItem.dataset.url) {
          $('#edit-url').val(selectItem.dataset.url);
          $('#edit-name').val(selectItem.dataset.title);
        }
      });

      $('#edit-display').append(tree);
      chrome.send('getBookmarkNodeByID', []);
    });

    $('#edit-topsite').on('click', function() {
      $('#edit-display').html('');

      $(this).siblings().removeAttr('sld');
      $(this).attr('sld', '');

      chrome.send('getTopSites', []);
    });

    chrome.send('getRecommendedWebsites', []);

    $('#edit-cancel').on('click', function() {
      $('#edit-page').css('display', 'none');
      $('#edit-name').val('');
      $('#edit-url').val('');
    });

    if (type == 'edit') {
      $('#edit-url').val(webList[index].url);
      $('#edit-name').val(webList[index].title);
    }

    $('#edit-save').on('click', function() {
      handleInfo(type, $('#edit-name').val(), $('#edit-url').val(), index);
    });

    document.getElementById('edit-url').addEventListener('keydown', function(e) {
      if (e.keyCode == 13) {
        handleInfo(type, $('#edit-name').val(), $('#edit-url').val(), index);
      }
    });
    document.getElementById('edit-name').addEventListener('keydown', function(e) {
      if (e.keyCode == 13) {
        handleInfo(type, $('#edit-name').val(), $('#edit-url').val(), index);
      }
    });
  }

  // 修改网址
  function editWebsite(name, url, index) {
    $('#edit-page').css('display', 'none');

    var node = $('#pages').children().eq(index).children('div');
    node.attr('title', name);
    node.attr('url', url);
    node.children('a').attr('href', url);
    node.children('a image').attr('src', waitThumbnail);
    node.children('p').children(':first').text(name);

    var websiteList = getCurrentData();
    websiteList[index] = {
      title: name,
      url: url,
      thumbnail: defaultThumbnail,
    }

    localStorage.setItem('webList', JSON.stringify(websiteList));
    chrome.send('getThumbnail', [parseInt(index), url]);
  }

  // 获取书签
  function getBookmarkNodeByID(id, data) {
    // var p = id ? "bookmarktreenode_" + id : "bookmarktree";
    // var p = document.getElementById(p);
    var p;
    if (!id) {
      p = document.getElementById("bookmarktree");
    } else {
      var condition = '[data-id=\'' + id + '\']';
      console.log(condition);
      p = document.querySelector(condition);
    }

    data.forEach(function(item, index) {
      var sub = new cr.ui.TreeItem({
        label: item.title,
        title: ""
      });
      sub.name = "bookmarktreenode_" + item.id;
      sub.dataset.id = item.id;
      sub.dataset.url = item.url;
      sub.dataset.title = item.title;
      if (item.url) {
        sub.labelElement.style.backgroundImage = 'url(nfsbrowser://favicon/' + item.url + ')';
      } else {
        sub.mayHaveChildren_ = true;
      }
      p.add(sub);
      sub.addEventListener('expand', function() {
        if (!this.hasLoad) {
          chrome.send('getBookmarkNodeByID', [this.dataset.id]);
        }
      });
    });
    p.expanded = true;
    p.hasLoad = true;
  }

  /*
    获取当前数据:
      1. 默认搜索引擎
      2. 导航网站
  */
  function getCurrentData() {
    return JSON.parse(localStorage.getItem('webList'));
  }

  // 获取推荐网站列表
  function getRecommendedWebsites(websiteList) {
    renderSelect(0, websiteList);
  }

  // 获取缩略图
  function getThumbnail(data) {
    var websiteList = getCurrentData();
    var index = data.index;
    var thumbnail = data.thumbnail;

    let lis = document.getElementById('pages').querySelectorAll('li');
    let li = lis[index];
    let image = li.querySelector('a img');
    image.src = thumbnail;

    websiteList[index].thumbnail = thumbnail;
    localStorage.setItem('webList', JSON.stringify(websiteList));
  }

  // 获取常用记录
  function getTopSites(data) {
    renderSelect(2, data);
  }

  // 处理导航页的右键菜单
  function handleContextMenu(e) {
    console.log(e);
    let index = $(this).attr('index');
    let url = $('#pages').children().eq(index).find('div').attr('url');

    if (e.target) {
      var number = e.target.getAttribute('index');
      switch(number) {
        case '0':
          chrome.send('openURL', [parseInt(0), url]);
          break ;

        case '1':
          chrome.send('openURL', [parseInt(1), url]);
          break;

        case '2':
          chrome.send('openURL', [parseInt(2), url]);
          break;

        case '3':
          editPage('edit', index);
          break;

        case '4':
          removePage(index);
          break;

        default:
          break;
      }
    }
  }

  function handleInfo(type, name, url, index) {
    var url_string, error;

    if (url === '' || url === 'http://') {
      $('#message').text(loadTimeData.getString('url_is_empty'));
      $('#message').css('display', 'block');
      setTimeout(function() {
        $('#message').css('display', 'none');
      }, 1500);
    } else if ( !(url_string = isURL(url) ? url : (isURL('http://' + url) ? ('http://' + url) : false)) ) {
      $('#message').text(loadTimeData.getString('url_invalid'));
      $('#message').css('display', 'block');
      setTimeout(function() {
        $('#message').css('display', 'none');
      }, 1500);
    } else {
      if (type === 'edit') {
        editWebsite(name, url_string, index);
      } else {
        addWebsite(name, url_string, index);
      }
    }
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
    keyWord.children().on('click', function() {
      $('#input').val($(this).html());
      console.log($('#input').val());
      keyWord.animate({
        height: 0
      }, 10, function() {
        keyWord.css('display', 'none');
        keyWord.css('height', 'auto');
        keyWord.html('');
      });

      $('#search-bt').click();
    });

    // 输入框失去焦点后收缩联想框
    $('#input').on('blur', function(){
      var keyWord = $('#keyword');
      setTimeout(function(){
        keyWord.animate({
          height: 0
        }, 100, function() {
          keyWord.css('display', 'none');
          keyWord.css('height', 'auto');
          keyWord.html('');
        });
      },100)
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
        numspan++;
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
  }

  function setThumbnail(loaded) {
    var webList = getCurrentData();
    for (var i = 0; i < pagesCount; ++i) {
      if (!webList[i])
        continue;

      let lis = document.getElementById('pages').querySelectorAll('li');
      let li = lis[i];
      let image = li.querySelector('a img');

      if (webList[i].status == '0') {
        image.src = webList[i].thumbnail;
      } else {
        chrome.send('getThumbnail', [parseInt(i), webList[i].url]);
      }
    }
  }

  // 绘制添加按钮
  function renderAdd(index) {
    let li = $('#pages').children().eq(index);
    li.empty();

    var div = $('<div></div>');
    div.attr('class', 'add-page');
    div.attr('type', 'new');
    div.attr('index', index);
    div.html('+');
    li.append(div);

    div.on('click', function() {
      var type = $(this).attr('type');
      var index = $(this).attr('index');
      editPage(type, index);
    });

    if (li.hasClass('link')) {
      li.removeAttr('class');
    }
  }

  // 绘制单元格
  function renderCell(index, url, title, thumbnail, status) {
    let p       = $('<p>');
    let a       = $('<a>').attr('href', url);
    let li      = $('#pages').children().eq(index).empty();
    let div     = $('<div>').attr('title', title).attr('url', url);
    let image   = $('<img>').attr('src', thumbnail).attr('class', 'thumbnail-container');
    let text    = $('<span>').text(title);
    let edit    = $('<span>').attr('index', index).attr('type', 'edit');
    let remove  = $('<span>').attr('index', index);

    // li.
    li.attr('class', 'link');
    li.append(div);
    div.append(a).append(p);
    a.append(image);
    p.append(text).append(edit).append(remove);

    li.on('contextmenu', renderContextMenu);

    div.on('mouseenter', function() {
      this.childNodes[1].style.display = "flex";
    });
    div.on('mouseleave', function() {
      this.childNodes[1].style.display = "none";
    });

    edit.on('click', function() {
      editPage($(this).attr('type'), $(this).attr('index'));
    });

    remove.on('click', function() {
      removePage($(this).attr('index'));
    });
  }

  // 绘制右键菜单
  function renderContextMenu(e) {
    e.preventDefault();

    var menu = $('#context-menu');
    menu.css({
      left: e.clientX + 'px',
      top: e.clientY + 'px',
      visibility: 'visible'
    });

    menu.attr('index', $(this).attr('index'));
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

  // 绘制导航窗口
  function renderNavigation() {
    let webList = getCurrentData();
    for (let i = 0; i < pagesCount; ++i) {
      if (webList[i]) {
        renderCell(i, webList[i].url, webList[i].title, webList[i].thumbnail, webList[i].status);
      } else {
        renderAdd(i);
      }
    }
    chrome.send('isDBLoaded', []);
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

  function renderSelect(index, websiteList) {
    switch(index) {
      case 0: {
        for (var i = 0; i < websiteList.length; ++i) {
          var p = $('<p>').text(websiteList[i].title)
                          .attr('title', websiteList[i].title)
                          .attr('url', websiteList[i].url);
          $('#edit-display').append(p);

          p.on('click', function() {
            if (!!$('#edit-display p[selected]')) {
              $('#edit-display p[selected]').removeAttr('selected');
            }

            $(this).attr('selected', '');
            $('#edit-url').val($(this).attr('url'));
            $('#edit-name').val($(this).attr('title'));
          });

          p.on('dblclick', function(e) {
            // To do ...
          });
        }
        break;
      }

      case 2: {
        for (var i = 0; i < websiteList.length; ++i) {
          var div = $('<div>').html(websiteList[i].title)
                              .attr('class', 'edit-display-content')
                              .attr('title', websiteList[i].title)
                              .attr('url', websiteList[i].url);
          div.css('background-image', 'url(nfsbrowser://favicon/' + websiteList[i].url + ')');

          div.on('click', function() {
            if (!!$('#edit-display div[selected]')) {
              $('#edit-display div[selected]').removeAttr('selected');
            }
            $(this).attr('selected', '');
            $('#edit-url').val($(this).attr('url'));
            $('#edit-name').val($(this).attr('title'));
          });

          div.on('dblclick', function(e) {
            // To do ...
          });

          $('#edit-display').append(div);
        }
        break;
      }

      default:
        break;
    }
  }

  function removePage(index) {
    var webList = getCurrentData();

    renderAdd(index);

    webList[index] = null;
    localStorage.setItem('webList', JSON.stringify(webList));
  }

/*********************************public***********************************************************/
  // 获取当前搜索引擎
  function getDefaultSearchEngine(searchEngine) {
    renderSearchBox(searchEngine);
  }

  // 获取默认网站列表
  function getDefaultWebsites(websiteList) {
    localStorage.setItem('webList', JSON.stringify(websiteList));
    renderNavigation();
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
    });
  }

  // Monitor search engine changes
  function onSearchEngineChanged(data) {
    $('#engine-logo').css('background-image', 'url(' + data.favicon + ')');
  }

  // 拖拽
  function drag() {
    $('#pages').dragsort({
      dragSelector: "div",
      dragBetween: true,
      dragEnd: function() {
        let websiteList = [];
        let pages = document.getElementById('pages').children;
        for (let i = 0; i < 8; i++) {
          if (pages[i].getAttribute('class') == null) {
            websiteList.push(null);
            continue;
          }

          websiteList.push({
            title: pages[i].firstChild.title,
            url: pages[i].firstChild.getAttribute('url'),
            thumbnail: $('#pages').children().eq(i).find('img').attr('src'),
            status: 0
          });
        }

        getDefaultWebsites(websiteList);
      },
      placeHolderTemplate: "<li class='placeHolder'><div></div></li>"
    });
  }

  function onLoad() {
    chrome.send('getDefaultSearchEngine', []);

    if (!localStorage.getItem('webList')) {
      chrome.send('getDefaultWebsites', []);
    } else {
      renderNavigation();
    }

    var hasCustomBg = loadTimeData.getBoolean("hasCustomBackground");
    document.documentElement.setAttribute('hascustombackground', hasCustomBg);

    $(document).on('click', function(e) {
      $('#context-menu').css('visibility', 'hidden');
    });
    $(document).on('contextmenu', function(e) {
      if (e.target && e.target.className === 'add-page') {
        e.preventDefault();
      }
    });

    // 处理导航页的右键菜单
    $('#context-menu').on('click', handleContextMenu);
    $('#context-menu').on('contextmenu', function(e) {
      e.preventDefault();
    });

    // 拖拽
    drag();
  }

  function setBackgroundOffset(offset) {
    document.documentElement.style.backgroundPosition = offset;
  }

  function themeChanged(themeData) {
    document.documentElement.setAttribute('hascustombackground',
                                          themeData.hasCustomBackground);
    document.documentElement.style.backgroundImage =
      'url(nfsbrowser://theme/IDR_THEME_NTP_BACKGROUND?' + themeData.themeId + ')';
  }

  return {
    getBookmarkNodeByID: getBookmarkNodeByID,
    getDefaultSearchEngine: getDefaultSearchEngine,
    getDefaultWebsites: getDefaultWebsites,
    getRecommendedWebsites: getRecommendedWebsites,
    getSearchEngines: getSearchEngines,
    getThumbnail: getThumbnail,
    getTopSites: getTopSites,
    handleKey: handleKey,
    onLoad: onLoad,
    onSearchEngineChanged: onSearchEngineChanged,
    setThumbnail: setThumbnail,
    setBackgroundOffset: setBackgroundOffset,
    themeChanged: themeChanged
  };
});

document.addEventListener('DOMContentLoaded', ntp.onLoad);


