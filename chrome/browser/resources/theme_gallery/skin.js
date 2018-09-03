cr.define('theme_gallery', function() {
  'use strict';

const originUrl = 'http://124.16.141.197:3000';
const skinUrl = 'http://124.16.141.197:3000/theme/';

function downloadThemeCrx(themeUrl) {
  let a = document.createElement('a');
  a.href = themeUrl;
  a.download = '';
  a.click();
}

function getSkinInfoEnd(request) {
  let data = JSON.parse(this.response);
  if (!!data['error']) {
    return ;
  }

  console.log(originUrl + data['theme']['file_path']);
  downloadThemeCrx(originUrl + data['theme']['file_path']);
}

function getSkinInfoError(error) {
  console.log(error);
}

function installTheme(e) {
  chrome.send('installTheme', [this.dataset.name]);
}

function getSkinEnd(xhr) {
  var data = JSON.parse(this.response);

  if (!!data['error']) {
    console.log(data['error']);
    return ;
  }

  var ul = document.createElement("ul");
  var fashion = document.querySelector("#fashion");

  var skin = data['theme'];
  for (var i in skin) {
    let li = document.createElement("li");
    let text = loadTimeData.getString("usetheme");
    let div = document.createElement('div');
    let span = document.createElement('span');
    let em = document.createElement('em');

    div.dataset.used = 0;
    span.dataset.name = skin[i]['crx_id'];
    span.innerText = loadTimeData.getString('usetheme');
    li.style.backgroundImage = "url(" + originUrl + skin[i]['img'] + ")";

    span.addEventListener('click', installTheme);

    div.appendChild(span);
    div.appendChild(em);
    li.appendChild(div);

    ul.appendChild(li);
  }

  ul.className = "skin-list";
  fashion.appendChild(ul);

  chrome.send('getCurrentTheme', []);
}

function getSkinError(error) {
  console.log(error);
}

function themeInstallComplete(themeId) {
  let skinList = document.querySelectorAll('[data-used="3"]');
  skinList.forEach(function(item, index) {
    item.dataset.used = 0;
  });


  let usedTheme = document.querySelector("[data-used = '1']");
  if (!!usedTheme) {
    usedTheme.dataset.used = 0;
    usedTheme.firstChild.innerHTML = loadTimeData.getString("usetheme");
  }

  let span = document.querySelector("[data-name=" + themeId + "]");
  if (!!span) {
    span.parentNode.dataset.used = 1;
  }
}

function init() {
  let xhr = new XMLHttpRequest();
  xhr.addEventListener('load', getSkinEnd);
  xhr.addEventListener('error', getSkinError);
  xhr.open('get', skinUrl, true);
  xhr.send();

  let defaultTheme = document.querySelector("[data-name='default']");
  if (!!defaultTheme) {
    defaultTheme.addEventListener('click', function() {
      chrome.send('installTheme', ['default']);
    });
  }
}

function setCurrentTheme(themeID) {
  var skinUsed = document.querySelector("[data-name = " + themeID + "]");
  skinUsed.parentNode.dataset.used = "1";
}

function installThemeFromServer(themeId) {
  // this.parentNode.dataset.used = 2;
  // this.innerHTML = '<img src=img/loading.png>';
  let usedTheme = document.querySelector("[data-name=" + themeId + "]");
  usedTheme.parentNode.dataset.used = 2;
  usedTheme.innerHTML = '<img src=img/loading.png>';

  let skinList = document.querySelectorAll('[data-used="0"]');
  skinList.forEach(function(item, index) {
    item.dataset.used = 3;
  });

  // let crxId = this.dataset.name;
  let url = skinUrl + themeId;

  let xhr = new XMLHttpRequest();
  xhr.addEventListener('load', getSkinInfoEnd);
  xhr.addEventListener('error', getSkinInfoError);
  xhr.open('GET', url, true);
  xhr.send();
}

return {
  themeInstallComplete : themeInstallComplete,
  setCurrentTheme : setCurrentTheme,
  installThemeFromServer: installThemeFromServer,
  init : init
}

});

document.addEventListener('DOMContentLoaded', theme_gallery.init);
