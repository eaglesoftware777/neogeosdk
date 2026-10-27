-- Reopen the saved source and emit per-layer/flattened pixel evidence.
local root=assert(app.params.root)
local spr=assert(app.open(root..'/maiya_walkrun.aseprite'))
assert(spr.width==80 and spr.height==64 and spr.colorMode==ColorMode.INDEXED)
local f=assert(io.open(root..'/poses.json','r'));local poses=json.decode(f:read('*a'));f:close()
assert(spr.transparentColor==0 and #spr.frames==#poses)
local info={width=spr.width,height=spr.height,indexed=true,transparentColor=spr.transparentColor,colors={},layers={},tags={}}
for i=0,15 do
  local c=spr.palettes[1]:getColor(i);info.colors[i+1]={c.red,c.green,c.blue,c.alpha}
end
for _,l in ipairs(spr.layers) do
  info.layers[#info.layers+1]={name=l.name,background=l.isBackground,visible=l.isVisible,editable=l.isEditable}
end
for _,t in ipairs(spr.tags) do info.tags[#info.tags+1]={name=t.name,from=t.fromFrame.frameNumber,to=t.toFrame.frameNumber} end
local function save(im,name)
  im:saveAs{filename=root..'/layers/'..name..'.png',palette=spr.palettes[1]}
  local raw=assert(io.open(root..'/layers/'..name..'.txt','w'))
  for y=0,63 do
    local row={};for x=0,79 do local v=im:getPixel(x,y);row[#row+1]=v==0 and '.' or string.format('%X',v) end
    raw:write(table.concat(row),'\n')
  end
  raw:close()
end
for i,p in ipairs(poses) do
  local flat=Image(80,64,ColorMode.INDEXED);flat:drawSprite(spr,i);save(flat,p.name..'__flat')
  for _,l in ipairs(spr.layers) do
    if l.name~='ref' then
      local im=Image(80,64,ColorMode.INDEXED);local c=l:cel(i)
      assert(c and c.opacity==255 and l.opacity==255)
      im:drawImage(c.image,c.position);save(im,p.name..'__'..l.name)
    end
  end
end
local out=assert(io.open(root..'/aseprite_source.json','w'));out:write(json.encode(info));out:close();spr:close()
print('Reopened source and exported every layer without resampling')
