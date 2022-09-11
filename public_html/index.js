var world = {};
var TYPES = [0, "player", "exit"];
function redraw() {
  const gameDiv = document.getElementById("game");
  let html = "";
  world.entities.forEach(e => {
    html += ("<div>entity id: "+e.id+", type: "+TYPES[e.type]+"</div>");
  })
  gameDiv.innerHTML = html;
}
document.addEventListener("DOMContentLoaded", () => {
  fetch("/state.json").then( (r) => {
    r.json().then((state) => {
      world = state;
      redraw();
    })
  })
})
