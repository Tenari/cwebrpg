function getRandomInt(min, max) {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min) + min); // The maximum is exclusive and the minimum is inclusive
}
const CANVAS_WIDTH = 800;
const CANVAS_HEIGHT = 608;
const FLOORS = [null, 'grass', 'stone'];
const GRASS_COLORS = ['#3b680d', '#3b680d', '#3b680d', '#68680d','#68680d', '#0d680d'];
const MOVE_FRAMES = 30;
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
let ctx, username, image;
var localState = {
  mode: "move",
  cursor: null,
  moving: {
  },
  retriggering: false,
};
var room = {};
var world = {};
var TYPES = [0, "player", "exit", 'fireball'];
const SPELL_KEYS = [0,'fireball', 'firestream', 'inferno'];
const SPELLS = {
  fireball: {
    name: 'Fireball',
    desc: 'target one space at arbitrary distance to fill with fire',
  },
  firestream: {
    name: 'Firestream',
    desc: 'target one cardinal direction to fill with fire for 5 spaces',
    distance: 5,
  },
  inferno: {
    name: 'Inferno',
    desc: 'fill all adjacent spaces to you for 2 spaces with fire',
    distance: 5,
  },
};
function redraw() {
  const factor = 32;

  ctx.fillStyle = '#ffffff';
  ctx.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

  const player = getUserPlayer();
  if (player.availableSpells && player.availableSpells.length > 0) {
    return drawSpellMenu(player);
  }

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

  let reTrigger = false;
  world.entities.forEach(e => {
    if(TYPES[e.type] == "player") {
      let dx = e.location.x*factor;
      let dy = e.location.y*factor;
      if (e.moveFrame && e.moveFrame > 0) {
        if (!localState.moving[e.id]) {
          localState.moving[e.id] = e.moveFrame;
        }
        if (localState.moving[e.id] < MOVE_FRAMES) {
          reTrigger = true;
        }
        const offset = (localState.moving[e.id] / MOVE_FRAMES) * factor;
        if (e.moveGoal.x > e.location.x) {
          dx += offset;
        } else if (e.moveGoal.x < e.location.x) {
          dx -= offset;
        } else if (e.moveGoal.y > e.location.y) {
          dy += offset;
        } else if (e.moveGoal.y < e.location.y) {
          dy -= offset;
        }
      } else {
        delete localState.moving[e.id];
      }
      // print the name
      drawText(ctx, e.name, dx, dy, "#0000ff");
      // block color
      if (e.dead) {
        ctx.fillStyle = '#878787';
        ctx.fillRect(dx, dy, factor, factor);
      } else {
        ctx.fillStyle = '#000000';
        ctx.drawImage(image, dx, dy, factor, factor);
      }
      if (e.health > 0) {
        ctx.fillStyle = 'red';
        ctx.fillRect(dx, dy, (e.health / e.maxHealth) * factor, 2);
      }
    } else {
      if (TYPES[e.type] == 'fireball'){
        ctx.fillStyle = '#ff0000';
      } else {
        ctx.fillStyle = '#00ff00';
      }
      ctx.fillRect(e.location.x*factor, e.location.y * factor, factor, factor);
    }
  })
  if (localState.mode == 'cast') {
    if (SPELL_KEYS[player.knownSpells[0]] == 'fireball') {
      drawEmptyBox(ctx, localState.cursor.x * factor, localState.cursor.y * factor, factor, factor);
      drawText(ctx, "Casting Fireball...", 9*factor, 1 * factor, "red", "20px Courier New");
    } else if (SPELL_KEYS[player.knownSpells[0]] == 'firestream') {
      const streamWidth = localState.cursor.y != player.location.y ? 1 : SPELLS.firestream.distance;
      const streamHeight = streamWidth == 1 ? SPELLS.firestream.distance : 1;
      drawEmptyBox(ctx, localState.cursor.x * factor, localState.cursor.y * factor, streamWidth * factor, streamHeight * factor);
      drawText(ctx, "Casting Firestream...", 9*factor, 1 * factor, "red", "20px Courier New");
    } else if (SPELL_KEYS[player.knownSpells[0]] == 'inferno') {
      drawEmptyBox(ctx, (localState.cursor.x-2) * factor, (localState.cursor.y-2) * factor, SPELLS.inferno.distance*factor, SPELLS.inferno.distance*factor);
      drawText(ctx, "Casting Inferno...", 9*factor, 1 * factor, "red", "20px Courier New");
    }
  }
  if (reTrigger && !localState.retriggering) {
    localState.retriggering = true;
    window.requestAnimationFrame(() => {
      for (let k of Object.keys(localState.moving)) {
        localState.moving[k] += 1;
      }
      localState.retriggering = false;
      redraw();
    })
  }
}

function drawEmptyBox(ctx, x, y, width, height, color, lineWidth) {
  ctx.beginPath();
  ctx.lineWidth = lineWidth || "2";
  ctx.strokeStyle = color || "red";
  ctx.rect(x, y, width, height);
  ctx.stroke();
}
function drawTile(ctx, x, y, size, color) {
  color = color || "#000000";
  ctx.fillStyle = color;
  ctx.fillRect(x, y, size, size);
}
function drawText(ctx, text, x, y, color, font) {
  font = font || "12px Courier New";
  color = color || "#000000";
  ctx.fillStyle = color;
  ctx.strokeStyle = color;
  ctx.font = font;
  ctx.fillText(text,x,y);
}
function getTextLines(ctx, text, maxWidth, font) {
  font = font || "12px Courier New";
  ctx.font = font;
  var words = text.split(" ");
  var lines = [];
  var currentLine = words[0];

  for (var i = 1; i < words.length; i++) {
    var word = words[i];
    var width = ctx.measureText(currentLine + " " + word).width;
    if (width < maxWidth) {
        currentLine += " " + word;
    } else {
        lines.push(currentLine);
        currentLine = word;
    }
  }
  lines.push(currentLine);
  return lines;
}
function drawSpellMenu(player) {
  if (localState.mode != "menu") {
    localState.mode = "menu";
    localState.menuCode = 'L'; // learn
    localState.menuPos = 0;
  }
  const margin = 50;
  const fontSize = 20;
  const font = fontSize + 'px Courier New';
  drawText(ctx, "Select which spell to learn:", margin, margin+fontSize, 'black', font);
  let yPos = 25;
  localState.menu = [];
  player.availableSpells.forEach(s => {
    let menuBox = {x: margin+10, width: CANVAS_WIDTH-(margin*2), y: yPos+margin};
    const key = SPELL_KEYS[s];
    const spell = SPELLS[key];
    drawText(ctx, spell.name+':', margin+10, margin+fontSize+yPos, 'black', font);
    yPos += (fontSize+5);
    const descFontSize = 16;
    const descFont = descFontSize + 'px Courier New';
    getTextLines(ctx, spell.desc, CANVAS_WIDTH-(margin*2), descFont).forEach(line => {
      drawText(ctx, line, margin+20, margin+descFontSize+yPos, 'black', descFont);
      yPos += (fontSize+3);
    })
    menuBox.height = yPos+margin - menuBox.y;
    localState.menu.push(menuBox);
  });

  const current = localState.menu[localState.menuPos];
  drawEmptyBox(ctx, current.x, current.y, current.width, current.height);
  localState.menuResult = localState.menuCode +' '+(player.availableSpells[localState.menuPos]);
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
    localState.spell = SPELL_KEYS[player.knownSpells[0]];
    localState.spellKey = player.knownSpells[0];
    if (localState.spell == 'firestream') {
      localState.cursor.y -= SPELLS.firestream.distance;
    }
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
  let username = prompt("Please enter your name less than 32 characters:", "");
  if (username != "" && username != null && username.length < 32) {
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
      if (localState.spell == 'fireball') {
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
      } else if (localState.spell == 'firestream') {
        const player = getUserPlayer();
        switch(dir){
          case 'N':
            localState.cursor.y = player.location.y - SPELLS.firestream.distance;
            localState.cursor.x = player.location.x;
            break;
          case 'S':
            localState.cursor.y = player.location.y + 1;
            localState.cursor.x = player.location.x;
            break;
          case 'E':
            localState.cursor.x = player.location.x + 1;
            localState.cursor.y = player.location.y;
            break;
          case 'W':
            localState.cursor.x = player.location.x - SPELLS.firestream.distance;
            localState.cursor.y = player.location.y;
            break;
        }
      }
      redraw();
    }
  } else if (localState.mode == 'menu') {
    const dir = VALID_INPUT[e.code];
    if (dir) {
      switch(dir){
        case 'N':
          localState.menuPos -= 1;
          if (localState.menuPos < 0) localState.menuPos = localState.menu.length - 1;
          break;
        case 'S':
          localState.menuPos += 1;
          if (localState.menuPos >= localState.menu.length) localState.menuPos = 0;
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

  image = new Image();
  image.src = 'wizard.png';
  //image.onload = function () {
  //    ctx.drawImage(image,5,5);
  //};

  let ws = new WebSocket("ws://"+window.location.host+"/chat");

  ws.onopen = function() {
    // Web Socket is connected. You can send data by send() method.
    console.log("connected...");
  };
  ws.onmessage = (event) => {
    // event.data is either a string (if text) or arraybuffer (if binary)
    world = JSON.parse(event.data);
    const player = getUserPlayer();
    if (player.xp && player.xp > (localState.xp || 0)) {
      localState.xp = player.xp;
      document.getElementById("xp").innerHTML = ""+player.xp;
    }
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
    if (localState.mode == "move") {
      if (VALID_INPUT[e.code]) {
        ws.send(VALID_INPUT[e.code]);
      }
    } else if (localState.mode == 'cast') {
      if (e.code == 'Enter') {
        ws.send('C '+localState.spellKey+' '+localState.cursor.x+" "+localState.cursor.y);
        toggleInputMode();
      }
    } else if (localState.mode == 'menu') {
      if (e.code == 'Enter') {
        ws.send(localState.menuResult);
        localState.mode = 'move';
      }
    }
    if (e.code == 'KeyQ') { //toggle mode
      toggleInputMode();
    }
  });
})
