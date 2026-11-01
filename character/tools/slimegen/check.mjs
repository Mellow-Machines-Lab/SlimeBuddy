// What the design's data must be before slimegen bakes it: a bad edit fails
// here, naming what's wrong, rather than baking a slime that can't move or
// sound as it should. The board's behaviour is pinned by its own tests
// (test_gel); these are the inputs' own rules.

const fail=(what)=>{throw Error('slimegen: '+what);};

// The pack's mood graph (character.json), over its lasting moods: every
// edge goes to another lasting mood, once; and the ordinary edges alone
// reach every lasting mood from every other.
function graphRules(character) {
  const lasting=character.moods.filter(m=>m.meaning),ids=lasting.map(m=>m.id),by=new Map(lasting.map(m=>[m.id,m]));
  for(const m of lasting){
    const to=[...m.moves.ordinary,...m.moves.dramatic];
    for(const next of to)if(!ids.includes(next)||next===m.id)fail(`${m.id} → ${next} isn't an edge to another lasting mood`);
    if(new Set(to).size!==to.length)fail(`${m.id} names an edge twice`);
    const reached=new Set([m.id]),pending=[m.id];
    while(pending.length)for(const next of by.get(pending.pop()).moves.ordinary)if(!reached.has(next))reached.add(next),pending.push(next);
    if(reached.size!==ids.length)fail(`the ordinary edges from ${m.id} reach ${reached.size} of ${ids.length} lasting moods`);
  }
}

// The expressions: one for each mood, of that mood, with its three
// gestures, its sound's pitch and gain, and only gags that exist; and the
// notice takes' faces, each an expression.
function expressionRules(graph,design,sound) {
  for(const id of Object.keys(graph.nodes)){
    const e=design.expressions[id];
    if(!e||e.mood!==id)fail(`${id} has no expression of its own`);
    if(!sound.moodProfiles[id])fail(`${id} has no sound profile`);
  }
  for(const [id,e] of Object.entries(design.expressions)){
    if(e.performances.length!==3)fail(`${id} has ${e.performances.length} gestures, not three`);
    for(const gag of e.gags||[])if(!design.gags[gag])fail(`${id} names no gag ${gag}`);
  }
  for(const n of design.notices)if(!design.expressions[n.face])fail(`${n.id} borrows no expression ${n.face}`);
}

// The pastimes' bodies: each its own id, and a gag or a recipe.
function pastimeRules(design) {
  const ids=design.pastimes.map(o=>o.id);
  if(new Set(ids).size!==ids.length)fail('two pastimes share an id');
  for(const o of design.pastimes){
    if(o.gag&&typeof o.gag!=='object')fail(`${o.id}'s gag isn't one of its own`);
    if(!o.gag&&!o.recipe)fail(`${o.id} has neither a gag nor a recipe`);
  }
}

// The sound sketches the board's effects are cut from: each the same from
// the same seed, starting from silence, finite, never louder than .8, at
// most 3 s, and silent at no gain.
function soundRules(sfx) {
  for(const name of Object.keys(sfx.recipes))for(const variant of [0,1,2]){
    const a=sfx.render(name,{variant,seed:53}),b=sfx.render(name,{variant,seed:53});
    if(a.data.some((v,i)=>v!==b.data[i]))fail(`${name} ${variant} renders differently from one seed`);
    const peak=a.data.reduce((m,v)=>Math.max(m,Math.abs(v)),0);
    if(a.data[0]!==0||!a.data.every(Number.isFinite)||!(peak>0&&peak<=.8)||!(a.duration>0&&a.duration<=3))fail(`${name} ${variant} isn't a clean sketch`);
    if(!sfx.render(name,{variant,gain:0}).data.every(v=>v===0))fail(`${name} isn't silent at no gain`);
  }
}

export function checkDesign({character,graph,design,sound,sfx}) {
  graphRules(character);
  expressionRules(graph,design,sound);
  pastimeRules(design);
  soundRules(sfx);
}
