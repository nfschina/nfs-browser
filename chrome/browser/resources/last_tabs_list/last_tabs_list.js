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

cr.define('ltl', function() {
  'use strict';

  let checkAllEl = undefined;
  let checkEl = undefined;

  function checkAll() {
    console.log("checkEl.length=" + checkEl.length);
    if (checkAllEl.checked) {
      for (var i = 0; i < checkEl.length; i++) {
        checkEl[i].checked = true;
        console.log(checkEl.length);
        console.log(checkEl[i].checked);
      }
    } else {
      for (var i = 0; i < checkEl.length; i++) {
        checkEl[i].checked = false;
        $("open_all_selected").disabled = true;
      }
    }
    updateButtonStatus();
  }

  function tabsListDOM(data) {
    var tabsListTemplate = '<div class="tab_container" id="tab_{{index}}">' +
      '<input type="checkbox" class="check" id="check_{{index}}"></input>' +
      "<div class='favicon' style='background-image: {{bgimage}};'></div>" +
      '<div class="title">' +
      '<a href="{{url}}" target="_blank" title="{{title}}" focus-type="title" tabindex="0">{{title}}</a>' +
      '</div>' +
      '</div>';

    var wrap = [];
    for (var i = 0; i < data.length; i++) {
      var backgroundImage = cr.icon.getFavicon(data[i].url);
      var _tabsListTemplate = tabsListTemplate.replace(/{{index}}/g, i)
        .replace(/{{url}}/, data[i].url)
        .replace(/{{bgimage}}/, backgroundImage)
        .replace(/{{title}}/g, data[i].title);

      wrap.push(_tabsListTemplate);
    }
    $("tabs_list").innerHTML = wrap.join('');
    checkEl = $("tabs_list").querySelectorAll(".check");
    checkAllEl = $("check_all");
    checkAllEl.checked = true;
    checkAll();
  }

  function updateButtonStatus() {
    var anyChecked = document.querySelectorAll('#tabs_list input:checked').length != 0;
    $('open_all_selected').disabled = !anyChecked;
    console.log("anyChecked=" + anyChecked);
  }

  function checkOne() {
    var flag = true;
    for (var i = 0; i < checkEl.length; i++) {
      if (!checkEl[i].checked) {
        flag = false;
      }
    }
    if (flag) {
      checkAllEl.checked = true;
    } else {
      checkAllEl.checked = false;
    }
    updateButtonStatus();
  }

  // function openAllSelected() {
  //   var selectedUrl = [];
  //   for (var i = 0; i < checkEl.length; i++) {
  //     if (checkEl[i].checked) {
  //       selectedUrl.push(testData[i].url);
  //       window.open(testData[i].url);
  //     }
  //   }
  // }

  function onRestore(list) {
    console.log(list);
    tabsListDOM(list);
    document.body.style.display = 'block';

    checkAllEl.addEventListener("click", checkAll);

    for (var i = 0; i < checkEl.length; i++) {
      checkEl[i].addEventListener("click", checkOne);
    }

    if ((!$('open_all_selected').disabled)) {
      $("open_all_selected").addEventListener("click", function() {
        let selectedUrl = [];
        for (let i = 0; i < list.length; ++i) {
          if (checkEl[i].checked) {
            selectedUrl.push(list[i]['url']);
            window.open(list[i]['url']);
          }
        }
      });
    }
  }

  function onLoad() {
    chrome.send('restore', []);
  }

  return {
    onLoad: onLoad,
    onRestore: onRestore,
  };
});

document.addEventListener('DOMContentLoaded', ltl.onLoad);
