const CANVAS_WIDTH = 800;
const CANVAS_HEIGHT = 600;
let ctx;
var world = {};
var TYPES = [0, "player", "exit"];
function redraw() {
  const factor = 10;
  ctx.fillStyle = '#ffffff';
  ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
  ctx.font = "12px Courier New";
  world.entities.forEach(e => {
    if(TYPES[e.type] == "player") {
      ctx.fillStyle = '#000000';
    } else {
      ctx.fillStyle = '#00ff00';
    }
    ctx.fillRect(e.location.x*factor, e.location.y * factor, factor, factor);
    ctx.fillStyle = "#0000ff";
    ctx.strokeStyle = "#0000ff";
    ctx.fillText(e.name, e.location.x*factor, e.location.y * factor);
  })
}
function update() {
  fetch("/updates.json?last_known="+world.lastUpdate).then(r => {
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
  ctx = document.getElementById("game").getContext('2d');

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
    const directions = {d: 'W', w: 'N', s: 'S', a: 'E'};
    if (directions[e.key]) {
      fetch("/move?u="+username+"&d="+directions[e.key]).then( r => { r.text().then(t => console.log(t))});
    }
  });
})
