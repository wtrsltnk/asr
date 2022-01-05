'use strict';

const routeParameterRegex = new RegExp('\{(?<ParameterName>[^\}]*)\}', 'g');

if (!String.prototype.endsWith) {
  String.prototype.endsWith = function(search, this_len) {
    if (this_len === undefined || this_len > this.length) {
      this_len = this.length;
    }
    return this.substring(this_len - search.length, this_len) === search;
  };
}


class DotcppClient {
    constructor(element){
        if (element === null) {
            throw 'element is null';
        }

        if (element.parentNode === null) {
            throw 'element.parentNode is null';
        }

        if (element.parentNode.parentNode === null) {
            throw 'element.parentNode.parentNode is null';
        }

        this.templates = [];
        this.configurations = [];
        this.targets = [];
        this.files = [];

        this.content = element.parentNode.parentNode;
        this.spinContainer = this.content.querySelector('.spin-container');
        if (this.spinContainer === null) {
            this.spinContainer = document.createElement('div');
            this.spinContainer.classList.add('spin-container');
            var spin = document.createElement('div');
            spin.classList.add('spin');
            this.spinContainer.appendChild(spin);
            this.content.appendChild(this.spinContainer);
        }
        this.responseContainer = this.content.querySelector('.response-container');
        if (this.responseContainer === null) {
            this.responseContainer = document.createElement('div');
            this.responseContainer.classList.add('response-container');
            this.responseContainer.classList.add('content-margin');

            var closeButton = document.createElement('button');
            closeButton.classList.add('close');
            closeButton.setAttribute('title', 'Close response');
            closeButton.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" fill="currentColor" class="bi bi-x" viewBox="0 0 16 16"><path d="M4.646 4.646a.5.5 0 0 1 .708 0L8 7.293l2.646-2.647a.5.5 0 0 1 .708.708L8.707 8l2.647 2.646a.5.5 0 0 1-.708.708L8 8.707l-2.646 2.647a.5.5 0 0 1-.708-.708L7.293 8 4.646 5.354a.5.5 0 0 1 0-.708z"/></svg>';
            closeButton.addEventListener('click', (e) => {
                this.responseContainer.parentNode.removeChild(this.responseContainer);
                this.responseContainer = null;
            });
            this.responseContainer.appendChild(closeButton);

            var h4 = document.createElement('h4');
            h4.innerText = 'Response';
            this.responseContainer.appendChild(h4);

            var pre = document.createElement('pre');
            this.responseContainer.appendChild(pre);
            this.content.appendChild(this.responseContainer);
        }
    }

    sendRequest(method, url) {
        this.xmlhttp = new XMLHttpRequest();

        var requestParameters = this.content.querySelectorAll('.request-parameter');
        url = this.updateUrl(url, requestParameters);
        this.xmlhttp.open(method, url, true);

        var postData = null;
        if (method.toLowerCase() === 'post') {

            for (var requestParameter of requestParameters) {
                if (requestParameter.name === 'body') {
                    postData = requestParameter.value;
                    this.xmlhttp.setRequestHeader("Content-Type", "application/json");
                }
            }
        }

        console.log(method, postData);

        return new Promise((resolve, reject) => {
           let x = this.xmlhttp;
           x.onreadystatechange = function() {
               if (x.readyState === XMLHttpRequest.DONE) {   // XMLHttpRequest.DONE == 4
                   if (x.status === 200) {
                       resolve(JSON.parse(x.responseText));
                   }
                   else if (x.status === 204) {
                       resolve('No Content');
                   }
                   else if (x.status === 400) {
                      const errorObject = {
                           status: 'Bad Request',
                           message: x.responseText
                      };
                     reject(errorObject);
                  }
                  else if (x.status === 404) {
                      const errorObject = {
                           status: 'Not Found'
                      };
                     reject(errorObject);
                  }
                  else {
                      const errorObject = {
                           status: x.status,
                           message: 'Something else other than 200 was returned'
                      };
                      reject(errorObject);
                  }
               }
           };

           x.send(postData);
        });
    }

    updateUrl(url, requestParameters) {
        let matches = url.matchAll(routeParameterRegex);

        for (var match of matches) {

            var found = [...requestParameters]
                .filter(function(element) {
                          return element.name === match.groups['ParameterName'];
                      });

            if (found.length === 0) {
                continue;
            }

            url = url.replace(match[0], found[0].value);
        }

        return url;
    }

    runRequestFromForm() {
        var aggregateRoot = this.content.querySelector('input[name="AggregateRoot"]');
        if (aggregateRoot === null) {
            return;
        }

        var url = '/api/' + aggregateRoot.value;

        this.runRequest('post', url);
    }

    runRequest(method, url) {
        this.responseContainer.querySelector('pre').innerText = '';
        this.responseContainer.style.display = 'none';
        this.responseContainer.parentNode.querySelector('.spin-container').style.display = 'block';

        this.sendRequest(method, url)
            .then(value=>
                  {
                      var pre = this.responseContainer.querySelector('pre');
                      pre.innerText = JSON.stringify(value, null, 2);
                      this.responseContainer.classList.remove('error');
                      this.responseContainer.style.display = 'block';
                      this.responseContainer.parentNode.querySelector('.spin-container').style.display = null;

                      if (url.endsWith('templates')) {
                          this.templates = value.map(t => t.name);
                      } else if (url.endsWith('configurations')) {
                          this.configurations = value;
                      } else if (url.endsWith('targets')) {
                          this.targets = value;
                      } else if (url.endsWith('files')) {
                          this.files = value;
                      }

                      this.updateInputFields();
                  })
            .catch(err =>
                   {
                       var pre = this.responseContainer.querySelector('pre');
                       pre.innerText = JSON.stringify(err, null, 2);
                       this.responseContainer.classList.add('error');
                       this.responseContainer.style.display = 'block';
                       this.responseContainer.parentNode.querySelector('.spin-container').style.display = null;
                   });
    }

    updateInputFields() {
        if (this.templates.length > 0) {
            var templateInputFields = document.querySelectorAll('input[name=template]');
            for (var templateElement of templateInputFields) {
                this.replaceInputWithSelect(templateElement, this.templates);
            }
        }

        if (this.configurations.length > 0) {
            var configurationInputFields = document.querySelectorAll('input[name=configuration]');
            for (var configElement of configurationInputFields) {
                this.replaceInputWithSelect(configElement, this.configurations);
            }
        }

        if (this.targets.length > 0) {
            var targetInputFields = document.querySelectorAll('input[name=target]');
            for (var targetElement of targetInputFields) {
                this.replaceInputWithSelect(targetElement, this.targets);
            }
        }

        if (this.files.length > 0) {
            var fileInputFields = document.querySelectorAll('input[name=file]');
            for (var fileElement of fileInputFields) {
                this.replaceInputWithSelect(fileElement, this.files);
            }
        }
    }

    replaceInputWithSelect(element, options) {
        var selectElement = document.createElement('select');
        selectElement.setAttribute('name', element.name);
        selectElement.classList.add('request-parameter');
        element.parentNode.insertBefore(selectElement, element);
        element.parentNode.removeChild(element);
        for (var o of options) {
            var option = document.createElement('option');
            option.value = o;
            option.innerText = o;
            selectElement.appendChild(option);
        }
    }
}

class Bootstrap {
    constructor(){
        var coll = document.getElementsByClassName("collapsible");
        var i;

        for (i = 0; i < coll.length; i++) {
          coll[i].addEventListener("click", function() {
              const vh = Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0);
              this.classList.toggle("active");
              var content = this.nextElementSibling;
              if (content.style.maxHeight){
                  content.style.maxHeight = null;
              } else {
                  content.style.maxHeight = (vh - 100) + "px";
              }
          });
        }

        var modals = document.getElementsByClassName("modal");

        for (var modal of modals) {
            var closeButtons = modal.querySelectorAll('*[data-close-modal="this"]');
            for (var closeButton of closeButtons) {
                closeButton
                    .addEventListener(
                        'click',
                        (e) => {
                            modal.style.display = 'none';
                        });
            }
        }

        var openModalButtons = document.querySelectorAll("button[data-open-modal]");
        for (var openModalButton of openModalButtons) {
            openModalButton
                .addEventListener(
                    'click',
                    (e) => {
                        var modal = document.getElementById(openModalButton.getAttribute('data-open-modal'));
                        if (modal === null) {
                            return;
                        }
                        modal.style.display = 'block';
                    });
        }
    }
}

const bootstrap = new Bootstrap();
