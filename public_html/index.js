function getRandomInt(min, max) {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min) + min); // The maximum is exclusive and the minimum is inclusive
}
const CANVAS_WIDTH = 800;
const CANVAS_HEIGHT = 600;
const FLOORS = [null, 'grass', 'stone'];
const GRASS_COLORS = ['#3b680d', '#3b680d', '#3b680d', '#68680d','#68680d', '#0d680d'];
let GRASS_ORDER = [];
(() => {
  for (let i = 0; i<100; i++) {
    GRASS_ORDER[i] = GRASS_COLORS[getRandomInt(0, GRASS_COLORS.length)];
  }
})();

const STONE_COLORS = ['#756f7c', '#7c7c6f'];
// TODO user can remap this
const VALID_INPUT = {
  'KeyW': 'N', //north
  'KeyD': 'E', //east
  'KeyS': 'S', //south
  'KeyA': 'W', //west
  'KeyP': 'P', //punch
};
let ctx, username;
var localState = {
  mode: "move",
  cursor: null,
};
var room = {};
var world = {};
var TYPES = [0, "player", "exit", 'fireball'];
function redraw() {
  const factor = 20;

  ctx.fillStyle = '#ffffff';
  ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

  const player = getUserPlayer();

  // draw room
  if (room && room.width && room.height && room.id == player.location.roomid) {
    for (let x = 0; x < room.width; x+=1) {
      for (let y = 0; y < room.height; y+=1) {
        let floorColor = null;
        if (FLOORS[room.floor[x][y]] === 'grass') {
          floorColor = GRASS_ORDER[(x+y) % GRASS_ORDER.length];
        }
        if (FLOORS[room.floor[x][y]] === 'stone') {
          floorColor = STONE_COLORS[(x+y) % STONE_COLORS.length];
        }
        drawTile(ctx, x*factor, y*factor, factor, floorColor);
      }
    }
  }

  if (player.dead) {
    drawText(ctx, "you are dead, loser. After ~10 seconds you will respawn.", factor*5, factor*5, "black", "20px Courier New");
    if (player.deathCount > 0) {
      document.getElementById("count").innerHTML = ""+player.deathCount;
    }
  }

  world.entities.forEach(e => {
    if(TYPES[e.type] == "player") {
      // print the name
      drawText(ctx, e.name, e.location.x*factor, e.location.y * factor, "#0000ff");
      // block color
      if (e.dead) {
        ctx.fillStyle = '#878787';
      } else {
        ctx.fillStyle = '#000000';
      }
    } else if (TYPES[e.type] == 'fireball'){
      ctx.fillStyle = '#ff0000';
    } else {
      ctx.fillStyle = '#00ff00';
    }
    ctx.fillRect(e.location.x*factor, e.location.y * factor, factor, factor);
  })
  if (localState.mode == 'cast') {
    ctx.beginPath();
    ctx.lineWidth = "2";
    ctx.strokeStyle = "red";
    ctx.rect(localState.cursor.x * factor, localState.cursor.y * factor, factor, factor);
    ctx.stroke();
    drawText(ctx, "Casting Fireball...", 9*factor, 1 * factor, "red", "20px Courier New");
  }
}

function drawText(ctx, text, x, y, color, font) {
  font = font || "12px Courier New";
  color = color || "#000000";
  ctx.fillStyle = color;
  ctx.strokeStyle = color;
  ctx.font = font;
  ctx.fillText(text,x,y);
}
function drawTile(ctx, x, y, size, color) {
  color = color || "#000000";
  ctx.fillStyle = color;
  ctx.fillRect(x, y, size, size);
}

function getUserPlayer(){
  return world.entities.find(e=>{
    return TYPES[e.type] == 'player' && e.name == username;
  });
}
function toggleInputMode() {
  localState.mode = localState.mode == "cast" ? 'move' : 'cast';
  if (localState.mode === 'cast') {
    const player = getUserPlayer();
    localState.cursor = {
      x: player.location.x,
      y: player.location.y
    };
  } else {
    localState.cursor = null;
  }
  redraw();
}
function update(usr) {
  longLivedDataStream(
    usr,
    newstate => {
      world = newstate;
      redraw();
    },
    newstate => {
      world = newstate;
      redraw();
      update(usr);
    }
  );
}
function parseTextResponse(response, cb) {
  let lines = response.split("\n\n");
  let parsed = null;
  let i = 0;
  while(i < lines.length && parsed === null) {
    try {
      parsed = JSON.parse(lines[lines.length - 1 - i]);
    } catch (e) {
    }
    i++;
  }
  if (!!parsed) {
    cb(parsed);
  }
}
function longLivedDataStream(username, cb, cb2) {
  const request = new XMLHttpRequest();
  const url = '/updates.json?u='+username;
  request.open('GET', url);
  request.responseType = "text";
  request.send();
  let lastGood = true; // seems like onprogress is triggering twice for every actual write from updates so skip every other
  request.onprogress = () => {
    if (lastGood) {
      parseTextResponse(request.response, cb);
    }
    lastGood = !lastGood;
  }
  request.onload = () => {
    parseTextResponse(request.response, cb2);
  };
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
    username = username.trim();
    setCookie("username", username);
  } else {
    alert('bad name');
    createUser();
  }
  return username;
}


function handleCursorMove(e) {
  if (localState.mode == 'cast') {
    const dir = VALID_INPUT[e.code];
    if (dir) {
      switch(dir){
        case 'N':
          localState.cursor.y -= 1;
          break;
        case 'S':
          localState.cursor.y += 1;
          break;
        case 'E':
          localState.cursor.x += 1;
          break;
        case 'W':
          localState.cursor.x -= 1;
          break;
      }
      redraw();
    }
  }
}


document.addEventListener("DOMContentLoaded", () => {
  ctx = document.getElementById("game").getContext('2d');

  username = getCookie('username').trim();
  if (username == '') {
    username = createUser();
  }

  let ws = new WebSocket("ws://"+window.location.host+"/chat");

  ws.onopen = function() {
    // Web Socket is connected. You can send data by send() method.
    console.log("connected...");
  };
  ws.onmessage = (event) => {
    // event.data is either a string (if text) or arraybuffer (if binary)
    world = JSON.parse(event.data);
    const player = getUserPlayer();
    if (player.location.roomid != room.id) {
      fetch("/room.json?id="+player.location.roomid).then( (r) => {
        r.json().then((state) => {
          room = state;
          redraw();
        })
      });
    }
    redraw();
  };
  document.addEventListener("keydown", (e) => {
    handleCursorMove(e);
  })
  document.addEventListener("keyup", (e) => {
    //console.log(e.code);
    if (localState.mode == "move") {
      if (VALID_INPUT[e.code]) {
        ws.send(VALID_INPUT[e.code]);
      }
    } else if (localState.mode == 'cast') {
      if (e.code == 'Enter') {
        ws.send('F '+localState.cursor.x+" "+localState.cursor.y);
        toggleInputMode();
      }
    }
    if (e.code == 'KeyQ') { //toggle mode
      toggleInputMode();
    }
  });
})
