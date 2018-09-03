cr.define('update', function() {
  'use strict';

  var updateEle = document.getElementById("update");
  var waitingEle = document.getElementById("waiting");
  var newestEle = document.getElementById("newest");
  var finishEle = document.getElementById("finish");
  var text1Ele = document.getElementById("waiting_text1");
  var text2Ele = document.getElementById("waiting_text2");
  var text3Ele = document.getElementById("waiting_text3");
  var retry = document.getElementById("retry_now");
  var wFinish = document.getElementById("w_finish");
  var lFinish = document.getElementById("l_finish");
  var refuseButton = document.getElementById("refuse");
  var lRefuseButton = document.getElementById("linux_refuse");
  var globalResult = undefined;

  var downloadUrl, name, os, ver, browserType, cpuArch;

  var retryCount = 0;

  //获取客户端操作系统类型
  function detectOS() {
    var sUserAgent = navigator.userAgent;
    var isWin = (navigator.platform == "Win32") || (navigator.platform == "Windows");
    var isMac = (navigator.platform == "Mac68K") || (navigator.platform == "MacPPC") || (navigator.platform == "Macintosh") || (navigator.platform == "MacIntel");
    if (isMac) return "Mac";
    var isUnix = (navigator.platform == "X11") && !isWin && !isMac;
    if (isUnix) return "Unix";
    var isLinux = (String(navigator.platform).indexOf("Linux") > -1);
    if (isLinux) return "Linux";
    if (isWin)  return "Windows";
    return "None";
  }

  function parseQueryString(url) {
    var obj = {};
    var keyvalue = [];
    var key = "",
        value = "";
    var paraString = url.substring(url.indexOf("?") + 1, url.length).split("&");
    for (var i in paraString) {
      keyvalue = paraString[i].split("=");
      key = keyvalue[0];
      value = keyvalue[1];
      obj[key] = value;
    }
    return obj;
  }

  function getPackageByInfo() {
    $.ajax({
      url: downloadUrl,
      type: "GET",
      dataType:'jsonp',
      data:{
        'version': ver,
        'os': os,
        'type': browserType,
        'arch': cpuArch
      },
      jsonp:'callback',
      timeout:10000,
      async: false,
      success:function(result) {
        retryCount = 0;

        if (result['error']) {
          return ;
        }
        var flag = result.IsLatestVersion ? true : false;
        if(flag){
          waitingEle.style.display='none';
          newestEle.style.display='block';
        }else{
          globalResult = result;
          chrome.send("packageExist");
        }
      },
      error: function(xhr, textStatus, errorThrown) {
        ++retryCount;
        if (retryCount < 3) {
          getPackageByInfo();
        } else {
          retryCount = 0;

          waitingEle.style.display='none';
          document.getElementById("error-message").innerText =
              loadTimeData.getString("net_error");
          retry.style.display='flex';
        }
      },
    });
  }

  function getOSAndVersion(){
    var info = parseQueryString(window.location.href);
    console.log(info);
    os = info['os'];
    ver = info['version'];
    document.querySelector(".content_top .version").innerHTML = ver ;
    var linuxTitle = document.getElementById("linux_os");
    var windowsTitle = document.getElementById("windows_os");
    linuxTitle.style.display="block";
    windowsTitle.style.display="none";

    if (info['name'] != "" && info['url'] != "") {
      name = info['name'];
      downloadUrl = info['url'];
      updateNow();
      return;
    }

    var update_url;
    if (!!info['ip']) {
      update_url = "http://" + info['ip'] + "/update/index.php";
    } else {
      update_url = "http://180.167.10.100/update/index.php";
    }

    browserType = info["type"];
    cpuArch = info["arch"];
    downloadUrl = update_url;

    getPackageByInfo();
  }

  function updateNow(){
    updateEle.style.display='none';
    waitingEle.style.display='block';
    text1Ele.style.display='none';
    if (os == "Windows") {
      text3Ele.style.display='none';
      text2Ele.style.display='inline';
    } else {
      text3Ele.style.display='inline';
      text2Ele.style.display='none';
    }
    chrome.send("updateNfs", [name, downloadUrl]);
  }

  function downloadComplete(exeAddress){
    waitingEle.style.display='none';
    finish.style.display='flex';

    if (os == "Windows") {
      wFinish.style.display="inline";
      lFinish.style.display="none";
      refuseButton.style.display="block";
      lRefuseButton.style.display="none";
    } else {
      wFinish.style.display="none";
      lFinish.style.display="inline";
      refuseButton.style.display="none";
      lRefuseButton.style.display="block";
    }
  }
  function downloadFailed(exeAddress) {
    document.getElementById("error-message").innerText =
        loadTimeData.getString("retrynow");
    waitingEle.style.display='none';
    retry.style.display='flex';
  }

  function retryUpdate(){
    retry.style.display='none';
    waitingEle.style.display='block';
    text1Ele.style.display = 'inline';
    text2Ele.style.display = 'none';
    text3Ele.style.display = 'none';
    getPackageByInfo();
  }

  function restartNow(){
    chrome.send("installUpdate");
  }
  function refuse(){
    chrome.send("closeDialog");
  }

  function packageExist(result) {
    if (result) {
      waitingEle.style.display='none';
      finish.style.display='flex';

      if (os == "Windows") {
        wFinish.style.display="inline";
        lFinish.style.display="none";
        refuseButton.style.display="block";
        lRefuseButton.style.display="none";
      } else {
        wFinish.style.display="none";
        lFinish.style.display="inline";
        refuseButton.style.display="none";
        lRefuseButton.style.display="block";
      }
    } else {
      if (!globalResult['url'] || !globalResult['name']) {
        return ;
      }

      waitingEle.style.display='none';
      updateEle.style.display='flex';

      name = globalResult['name'];
      downloadUrl = globalResult['url'];
    }
  }

  var updateButton = document.getElementById("update_now");
  updateButton.addEventListener("click", updateNow);

  var restartButton = document.getElementById("restart");
  restartButton.addEventListener("click", restartNow);

  refuseButton.addEventListener("click", refuse);

  lRefuseButton.addEventListener("click", refuse);

  var retryButton = document.getElementById("retry");
  retryButton.addEventListener("click", retryUpdate);

  return {
    downloadComplete: downloadComplete,
    getOSAndVersion:getOSAndVersion,
    downloadFail: downloadFailed,
    packageExist: packageExist,
  };
});

document.addEventListener('DOMContentLoaded', update.getOSAndVersion);
