BEGIN{
count=0;
}
{
	if ($5=="type")
	{
		if ($6==2 )
		  count ++;	
	}		
}
END{
	print count
}