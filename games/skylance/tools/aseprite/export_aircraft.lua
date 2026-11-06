local root=assert(app.params.root)
local f=assert(io.open(root..'/aircraft_models.json','r'));local models=json.decode(f:read('*a'));f:close()
for _,m in ipairs(models) do
 local spr=assert(app.open(root..'/aircraft_'..m.name..'.aseprite'))
 assert(spr.colorMode==ColorMode.INDEXED and spr.width==48 and spr.height==48 and spr.transparentColor==0)
 for _,l in ipairs(spr.layers) do assert(not l.isBackground and l.opacity==255) end
 for i,c in ipairs(m.palette) do local p=spr.palettes[1]:getColor(i-1);assert(p.red==c[1] and p.green==c[2] and p.blue==c[3] and p.alpha==(i==1 and 0 or 255)) end
 for i,pose in ipairs(m.frames) do
  local im=Image(48,48,ColorMode.INDEXED);im:drawSprite(spr,i)
  local name=root..'/frames/aircraft_'..m.name..'__'..pose.name
  im:saveAs{filename=name..'.png',palette=spr.palettes[1]}
  local raw=assert(io.open(name..'.txt','w'))
  for y=0,47 do local row={};for x=0,47 do row[#row+1]=string.format('%X',im:getPixel(x,y)) end;raw:write(table.concat(row),'\n') end;raw:close()
  if i<=7 then
   local layer;for _,l in ipairs(spr.layers) do if l.name=='shadow' then layer=l end end
   local cel=assert(layer:cel(i));local shadow=Image(48,48,ColorMode.INDEXED)
   for y=0,47 do for x=0,47 do local p=cel.image:getPixel(x,y);if p~=0 then assert(p>=17 and p<=31);shadow:putPixel(x,y,p-16) end end end
   local pal=Palette(16);pal:setColor(0,Color{r=0,g=0,b=0,a=0})
   for k=1,15 do pal:setColor(k,spr.palettes[1]:getColor(16+k)) end
   shadow:saveAs{filename=root..'/frames/shadow_'..m.name..'_'..(i-1)..'.png',palette=pal}
  end
 end
 spr:close()
end
print('Reopened saved aircraft palettes and exported native indexed frames')
