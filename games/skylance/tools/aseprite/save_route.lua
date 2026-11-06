local root=assert(app.params.root);local name=assert(app.params.name)
local f=assert(io.open(root..'/'..name..'_document.json','r'));local m=json.decode(f:read('*a'));f:close()
local P=dofile(assert(app.params.scripts)..'/pixel_tools.lua')
local spr=P.create(m.width,m.height,m.palette);spr.layers[1].name='terrain';local guides=spr:newLayer();guides.name='sector_guides';guides.isVisible=false;guides.isEditable=false
local im=Image(m.width,m.height,ColorMode.INDEXED);f=assert(io.open(root..'/'..name..'_indices.bin','rb'))
for y=0,m.height-1 do local row=assert(f:read(m.width));for x=0,m.width-1 do im:putPixel(x,y,row:byte(x+1))end end;f:close();spr:newCel(spr.layers[1],1,im,Point(0,0))
spr:saveAs(root..'/'..name..'.aseprite');spr:close()
-- Round-trip through the actual saved indexed editor document.
spr=assert(app.open(root..'/'..name..'.aseprite'));assert(spr.colorMode==ColorMode.INDEXED and spr.transparentColor==0)
im=Image(spr.width,spr.height,ColorMode.INDEXED);im:drawSprite(spr,1);im:saveAs{filename=root..'/'..name..'_editor.png',palette=spr.palettes[1]}
f=assert(io.open(root..'/'..name..'_editor_indices.bin','wb'));for y=0,spr.height-1 do local row={};for x=0,spr.width-1 do row[#row+1]=string.char(im:getPixel(x,y))end;f:write(table.concat(row))end;f:close();spr:close()
print('Saved/reopened native indexed terrain document: '..name)
