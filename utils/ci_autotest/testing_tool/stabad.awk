BEGIN{
  count=0;
}
{
  if ($2=="addr")
  {
      count++
      print "PBN =",$7
      
  } 
}
END{
  print "Total bad block =",count
}


