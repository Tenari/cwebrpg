var world = {};
var TYPES = [0, "player", "exit"];
function redraw() {
  const gameDiv = document.getElementById("game");
  let html = "";
  world.entities.forEach(e => {
    html += ("<div>entity id: "+e.id+", name: "+e.name+", type: "+TYPES[e.type]+", x: "+e.location.x+"</div>");
  })
  gameDiv.innerHTML = html;
}
function update() {
  fetch("/updates.json").then(r => {
    r.json().then(s => {
      world = s;
      redraw();
      update();
    })
  })
}
function setCookie(name, value) {
  document.cookie = name + "=" + value + ";path=/";
}
function getCookie(name) {
  name = name + "=";
  let decodedCookie = decodeURIComponent(document.cookie);
  let ca = decodedCookie.split(';');
  for(let i = 0; i <ca.length; i++) {
    let c = ca[i];
    while (c.charAt(0) == ' ') {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return c.substring(name.length, c.length);
    }
  }
  return "";
}
function deleteCookie(name) {
  document.cookie = name+"=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/"
}

function createUser() {
  let username = prompt("Please enter your name:", "");
  if (username != "" && username != null) {
    setCookie("username", username);
  } else {
    alert('bad name');
    createUser();
  }
  return username;
}

document.addEventListener("DOMContentLoaded", () => {
  let username = getCookie('username');
  if (username == '') {
    username = createUser();
  }
  fetch("/render.json?u="+username).then( (r) => {
    r.json().then((state) => {
      world = state;
      console.log(state);
      redraw();
      update();
    })
  })
  document.addEventListener("keyup", (e) => {
    if (e.key === "d") {
      fetch("/moveright").then( r => { r.text().then(t => console.log(t))});
    }
  });
})
