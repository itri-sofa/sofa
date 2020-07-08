BEGIN{
  count=0;
  addcount=0;
}
{
  if ($1=="PBN")
  {
      if( ($3 >= (v1*v3+v2)) || ($3<(v1*v3)) )
      {
          addcount++
      }    
  } 
}
END{
  if (count==0 && addcount==0)
    print "Mark bad block",v1,"with length",v2,"ok"
  else 
    if (count>0)
      print "Mark bad block",v1,"with length",v2,"is length fail"
    if (addcount>0)
      print "Mark bad block",v1,"with length",v2,"is position fail"   
}


