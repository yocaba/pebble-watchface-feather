(function() {
  loadOptions();
  submitHandler();
})();

function submitHandler() {
  var $submitButton = $('#submitButton');

  $submitButton.on('click', function() {
    console.log('Submit');

    var return_to = getQueryParam('return_to', 'pebblejs://close#');
    document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
  });
}

function loadOptions() {

  if (localStorage.lightColorScheme) {
    colorSchemeSelection[0].selected = localStorage.lightColorScheme === 'true';
    colorSchemeSelection[1].selected = localStorage.lightColorScheme === 'false';
    degreeUnitSelection[0].selected = localStorage.degreeCelsius === 'true';
    degreeUnitSelection[1].selected = localStorage.degreeCelsius === 'false';
  }
}

function getAndStoreConfigData() {

  var options = {
    lightColorScheme: colorSchemeSelection[0].selected,
    degreeCelsius: degreeUnitSelection[0].selected
  };

  localStorage.lightColorScheme= options.lightColorScheme;
  localStorage.degreeCelsius= options.degreeCelsius;

  console.log('Got options: ' + JSON.stringify(options));
  return options;
}

function getQueryParam(variable, defaultValue) {
  var query = location.search.substring(1);
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    if (pair[0] === variable) {
      return decodeURIComponent(pair[1]);
    }
  }
  return defaultValue || false;
}
