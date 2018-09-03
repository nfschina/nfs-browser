// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var g_username;
var g_password;

function browserSelected(path) {
  $(".new-path").val(path);
  chrome.send('getDiskSpace', [path]);
}

function downloadPromptd(url, name, path, username, password) {
  $(".new-url").val(url);
  $(".new-name").val(name);
  $(".new-path").val(path);
  g_username = username;
  g_password = password;  
  chrome.send('getDiskSpace', [path]);
}

function downloadDirectory(path) {
    let li = document.createElement("li");
    li.innerHTML = path;
    li.addEventListener('click',function() {
      var path = $(this).text();
      $(".new-path").val(path);
      $(".select-path").css("display","none");
      chrome.send('getDiskSpace', [path]);
    });
    document.querySelector(".select-path ul").appendChild(li);
}

function newDownloadDiskSpaceGet(result)
{
var f = parseFloat(result);
var size = loadTimeData.getString('remain')+' '+ f.toFixed(2) + 'GB';
jQuery(".downinfosize span").text(size);
}

function newDownloadFilenameGet(result)
{
   jQuery(".new-name").val(result);
   jQuery(".new-name").removeAttr("empty");
   jQuery("#ok").removeClass('d');
}

function newDownloadParameterCheckout(result, info)
{
   if (result == "OK")
   {
       $(".newdownload").css("display","none");
       $(".newform :input").val("");
       window.close();
   }
   else
   {
      jQuery(".newsubmit em").text(info)
      jQuery(".newsubmit em").removeAttr('hidden');

      if (result == "url")
      {
         $(".new-url").attr("empty","");
      }
      else if (result == "name")
      {
         $(".new-name").attr("empty","");
      }
      else if (result == "path")
      {
         $(".new-path").attr("empty","");
      }
   }
}

(function($){

$(function() {
  chrome.send('newDownloadLoad');
});

$(".newsubmit .ok").click(function(){
  var url = $(".new-url").val();
  var name = $(".new-name").val();
  var path = $(".new-path").val();
  var username = g_username;
  var password = g_password;

  chrome.send('newDownloadOk', [url, name, path, username, password]);
});

$(".newsubmit .cancel").click(function() {
  $(".newdownload").css("display","none");
  chrome.send('newDownloadCancel');
  window.close();
});

$(".newform .new-browse").click(function() {
  jQuery(".newsubmit em").attr('hidden','');
  chrome.send('newDownloadBrowser');
});

//$(".new-path").focus(function() {
//  $(".select-path").css("display","block");
//});

$(".newform .new-path").click(function() {
  jQuery(".newsubmit em").attr('hidden','');
  if ($(".select-path").is(":hidden")) {
    $(".select-path").css("display","block");
  } else {
    $(".select-path").css("display","none");
  }
});

$(".new-path").blur(function() {
  jQuery(".newsubmit em").attr('hidden','');
  setTimeout(function() {
    var len = $(".select-path li:focus").length;
    if (len == 0) {
      $(".select-path").css("display","none");
    }
  },150);
});

$(".new-url").blur(function(){
  jQuery(".newsubmit em").attr('hidden','');
  if($(this).val() == ""){
    $(this).attr("empty","");
    $("#ok").addClass('d');
  }else{
    var name = $(".new-name").val();
    var url = $(".new-url").val();
    var path = $(".new-path").val();
    if (name == '' || name == undefined || name == null)
    {
       chrome.send('getDownloadName', [url, path]);
    }
    
    $(this).removeAttr("empty");
    $("#ok").removeClass('d');    
  }
});

$(".new-name").blur(function(){
  jQuery(".newsubmit em").attr('hidden','');
  if($(this).val() == ""){
    $(this).attr("empty","");
    $("#ok").addClass('d');
  }else{
    $(this).removeAttr("empty");
    $("#ok").removeClass('d');
  }
});

$(".new-name").keyup(function(){
  if($(this).val() == ""){
    $("#ok").addClass('d');
  }else{
    $("#ok").removeClass('d');
  }
});

$(".new-url").focus(function() {
  jQuery(".newsubmit em").attr('hidden','');
});

$(".new-name").focus(function() {
  jQuery(".newsubmit em").attr('hidden','');
});

$(".new-path").focus(function() {
  jQuery(".newsubmit em").attr('hidden','');
});

})(jQuery);

window.oncontextmenu  = function(){
  var l = document.querySelector('.new-url:focus').length + 
    document.querySelector('.new-name:focus').length;
  if(l == 0){
    return false;
  }

  l = null;
}
