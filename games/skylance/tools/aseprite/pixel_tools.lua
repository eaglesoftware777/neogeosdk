-- Native mechanical pixel primitives. Writes remain on the source grid.
local P={}
function P.dot(im,x,y,c) assert(x>=0 and x<im.width and y>=0 and y<im.height,'authored contour clips canvas');im:putPixel(x,y,c) end
function P.line(im,x,y,a,b,c)
 local dx=math.abs(a-x);local sx=x<a and 1 or -1;local dy=-math.abs(b-y);local sy=y<b and 1 or -1;local e=dx+dy
 while true do P.dot(im,x,y,c);if x==a and y==b then break end;local t=2*e;if t>=dy then e=e+dy;x=x+sx end;if t<=dx then e=e+dx;y=y+sy end end
end
function P.rect(im,x,y,a,b,c) for yy=y,b do for xx=x,a do P.dot(im,xx,yy,c) end end end
function P.poly(im,v,c)
 local lo,hi=im.height-1,0;for _,p in ipairs(v) do lo=math.min(lo,p[2]);hi=math.max(hi,p[2]) end
 for y=lo,hi do local xs={};for i,a in ipairs(v) do local b=v[i%#v+1];if(a[2]<=y and b[2]>y)or(b[2]<=y and a[2]>y)then xs[#xs+1]=a[1]+(y-a[2])*(b[1]-a[1])/(b[2]-a[2])end end;table.sort(xs);for i=1,#xs-1,2 do for x=math.ceil(xs[i]),math.floor(xs[i+1])do P.dot(im,x,y,c)end end end
 for i,a in ipairs(v)do local b=v[i%#v+1];P.line(im,a[1],a[2],b[1],b[2],c)end
end
function P.panel(im,x,y,w,h,paint)
 P.rect(im,x,y,x+w-1,y+h-1,1);P.rect(im,x+1,y+1,x+w-2,y+h-2,paint or 4)
 P.line(im,x+1,y+1,x+w-2,y+1,6);P.line(im,x+1,y+2,x+1,y+h-2,5);P.line(im,x+w-2,y+2,x+w-2,y+h-2,2)
end
function P.create(w,h,palette)
 local s=Sprite(w,h,ColorMode.INDEXED);s.transparentColor=0;if s.layers[1].isBackground then app.layer=s.layers[1];app.command.LayerFromBackground()end
 for i,c in ipairs(palette)do s.palettes[1]:setColor(i-1,Color{r=c[1],g=c[2],b=c[3],a=i==1 and 0 or 255})end
 return s
end
return P
