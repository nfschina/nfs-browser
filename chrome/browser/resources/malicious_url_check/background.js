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

function IsURL(str_url) {
  var strRegex = "^nfsbrowser://";
  if(str_url === undefined)//if undefined,"search" is not defined to make error====refresh website will be occured in this situations
    return (false);
  else {
    if(str_url.search(strRegex))//match nfsbrowser
      return (true);
    else//not match
      return (false);
  }
}






function checkurlfunc(inputurl,tabid) {
  chrome.checkmurl.checkurl(inputurl,function callback(getdata) {
    if(getdata === 0) {
      console.log("No Virus\n");
    }else {
        console.log("tabid......",tabid);
		    chrome.tabs.executeScript(tabid,{code:'window.stop',});
        chrome.tabs.sendRequest(tabid,{messaging:"testfunc"},function handler(response) {
        console.log("extension test...");
        });
        console.log("#######forbidden######");
    }
  });
}


chrome.tabs.onUpdated.addListener(function(tabId, info) {
	if ((info.status === 'loading') && IsURL(info.url)) {
    console.log("tabId....",tabId);
    checkurlfunc(info.url,tabId);
  }
  else
    return;
});
