local root=assert(app.params.root);local f=assert(io.open(root..'/production_assets.json','r'));local assets=json.decode(f:read('*a'));f:close()
local docs={}
for _,a in ipairs(assets)do
 local s=docs[a.source];if not s then s=assert(app.open(root..'/'..a.source));docs[a.source]=s;assert(s.colorMode==ColorMode.INDEXED and s.transparentColor==0);for _,l in ipairs(s.layers)do assert(not l.isBackground and l.opacity==255)end end
 local im=Image(s.width,s.height,ColorMode.INDEXED);im:drawSprite(s,a.frame);im:saveAs{filename=root..'/'..a.png,palette=s.palettes[1]}
 local raw=assert(io.open(root..'/frames/'..a.name..'.txt','w'));for y=0,s.height-1 do local row={};for x=0,s.width-1 do local p=im:getPixel(x,y);assert(p<=15);row[#row+1]=string.format('%X',p)end;raw:write(table.concat(row),'\n')end;raw:close()
 if a.frame==1 and (a.kind=='enemy' or a.kind=='miniboss' or a.kind=='boss')then
  local l;for _,v in ipairs(s.layers)do if v.name=='shadow'then l=v end end;local cel=assert(l:cel(1));local sh=Image(s.width,s.height,ColorMode.INDEXED)
  for y=0,s.height-1 do for x=0,s.width-1 do local p=cel.image:getPixel(x,y);if p~=0 then sh:putPixel(x,y,p-16)end end end
  local pal=Palette(16);pal:setColor(0,Color{r=0,g=0,b=0,a=0});for i=1,15 do pal:setColor(i,s.palettes[1]:getColor(16+i))end
  sh:saveAs{filename=root..'/frames/shadow_'..a.name..'.png',palette=pal}
 end
end
for _,s in pairs(docs)do s:close()end
print('Reopened and exported native production sources')
