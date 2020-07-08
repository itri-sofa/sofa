BEGIN{
  count=0;
}
{
  if ($4=="LBA=")
  {
    if ( ($5 >= (v1*8)) && ($5 < (v1*8+v2)) )
    {
      count++      
      print "LBA =",$5,"Reason =",$8, "OK"
    }
    else
      print "LBA =",$5, "Address Fail"
      
  } 
}
END{
  print "Total error number =",count
}


