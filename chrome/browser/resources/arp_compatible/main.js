if (window.frameElement) {
  if ((window.frameElement.outerHTML &&
    window.frameElement.outerHTML.includes('expression(autoResize'))) {
    window.frameElement.style.height = window.document.body.scrollHeight;
    if (window.frameElement.name == 'result' &&
        window.parent &&
        window.parent.frameElement &&
        window.parent.frameElement.id == 'result9') {
      window.parent.frameElement.style.height =
          window.parent.document.body.scrollHeight;
    }
  }

  var parent = window.frameElement.parentElement;
  if (window.frameElement.style.height == '100%' && parent &&
      parent.tagName == 'DIV') {
    parent.style.height = '100%';
  }
}

var trList = document.getElementsByTagName('tr');
for (var i = 0; i < trList.length; i++) {
  if (trList[i].style.display == 'block') {
    trList[i].style.display = '';
  }
}

var inputDZFP = document.querySelector("input[type='text'][id='dzfp'][name='dzfp']");
if (inputDZFP) {
  inputDZFP.name = 'DZFP';
}

//For http://www.12333sh.gov.cn/index.shtml charset
if (window.location.host == "www.12333sh.gov.cn") {
  var forms = document.getElementsByTagName('FORM');
  for (var i = 0; i < forms.length; i++) {
    forms[i].acceptCharset="utf-8";
  }
}

// for http://law.npc.gov.cn/
if (window.location.host == "law.npc.gov.cn") {
  var label = document.getElementById('text_label');
  if (label) {
    label.style.float="left";
    label.style.width = "600px";
  };
}