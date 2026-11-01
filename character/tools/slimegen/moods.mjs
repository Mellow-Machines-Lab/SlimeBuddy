// MOODS.md, the design's moods to read: each family's moods, with their
// meanings, faces, bodies, gestures, comedy and moves, from the graph, the
// expressions and the pack's character.json, whose are the meanings and
// moves.
export function moodsDocument(graph,catalog,character) {
  const pack=new Map(character.moods.map(m=>[m.id,m]));
  let md='# Slime mood inventory and directed traversal\n\nGenerated from [emotion-graph.json](emotion-graph.json), [expressions.json](expressions.json)\nand the pack\'s [character.json](../../character.json) by `node character/tools/slimegen/slimegen.mjs`;\nedit those, not this.\n\nOrdinary neighbors still require context/pacing. Dramatic neighbors require fresh\nstrong edge-specific evidence. Staying is implicit at every node. Families and\nintensity labels are static design metadata, not LLM-selected numeric controls.\nFaces are V3 review candidates; body descriptions below are choreography targets.\n\n';
  for(const group of Object.values(graph.families)){
    md+='## '+group.label+'\n\n';
    for(const id of group.moods){
      const n=graph.nodes[id],e=catalog.expressions[id];
      md+='### '+e.label+' (`'+id+'`)\n\n';
      const m=pack.get(id);
      md+=m.meaning?'**Meaning:** '+m.meaning+'\n\n':'**Face only:** settles into `'+m.settles+'`.\n\n';
      md+='**Strength / activation / persistence:** '+n.intensityWithinFamily+' / '+n.activation+' / '+n.temporalClass+'.\n\n';
      md+='**Face:** '+e.art.note+'\n\n';
      md+='**Body intent:** '+n.bodyPerformance+'\n\n';
      md+='**Current gesture recipes:** '+e.performances.map(p=>p.label+' ('+p.duration+' s)').join('; ')+'.\n\n';
      if(e.gags)md+='**Its own comedy, idle only:** '+e.gags.map(id=>catalog.gags[id].label+' ('+catalog.gags[id].duration+' s)').join('; ')+'.\n\n';
      md+='**Entry evidence:** '+n.entryCues.join('; ')+'.\n\n';
      if(m.moves){
        md+='**Ordinary →** '+m.moves.ordinary.map(v=>'`'+v+'`').join(', ')+'.\n\n';
        md+='**Dramatic →** '+(m.moves.dramatic.length?m.moves.dramatic.map(v=>'`'+v+'`').join(', '):'none')+'.\n\n';
      }
    }
  }
  md+='## Idle comedy every mood shares\n\nThe face deals these as pastimes from `tier1` (render/gel/pastimes.cpp; CONTINUITY.md, Comedy):\n\n';
  const owned=new Set(Object.values(catalog.expressions).flatMap(e=>e.gags||[]));
  for(const id of Object.keys(catalog.gags).filter(id=>!owned.has(id)))md+='- `'+id+'`: '+catalog.gags[id].label+' ('+catalog.gags[id].duration+' s).\n';
  md+='\n## Reference face alternates (not graph nodes)\n\n';
  for(const [id,e] of Object.entries(catalog.expressions).filter(([id])=>id.includes(':')))md+='- `'+id+'` → mood `'+e.mood+'`: '+e.art.note+'\n';
  md+='\n## Supplied references and their mappings\n\n';
  for(const r of catalog.referenceMapping)md+='- '+r.reference+' → expression `'+r.expression+'`, mood `'+r.mood+'`'+(r.relatedMood?' (related `'+r.relatedMood+'`)':'')+'.\n';
  return md;
}
