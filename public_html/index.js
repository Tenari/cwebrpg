var world = {};
var TYPES = [0, "player", "exit"];
function redraw() {
  const gameDiv = document.getElementById("game");
  let html = "";
  world.entities.forEach(e => {
    html += ("<div>entity id: "+e.id+", type: "+TYPES[e.type]+", x: "+e.location.x+"</div>");
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
document.addEventListener("DOMContentLoaded", () => {
  fetch("/render.json").then( (r) => {
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
