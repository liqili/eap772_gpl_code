
function ipToInt(str_ip)
{
    var patternIp = /^\s*[0-9]{1,3}\.{1}[0-9]{1,3}\.{1}[0-9]{1,3}\.{1}[0-9]{1,3}\s*$/;
    var ip_array = str_ip.split(".");
    if(ip_array.length != 4)
    {
        return -1;      
    }
    if(!patternIp.test(str_ip))
    {
        return -1;
    }
    return (Number(ip_array[0])*(1<<24)+(Number(ip_array[1])<<16 |Number(ip_array[2])<<8 |Number(ip_array[3])));
}

function isNetIp(str_ip,str_inf_ip,str_inf_mask)
{
	var int_inf_ip = (ipToInt(str_inf_ip)&0xFFFFFFFF);
	var int_inf_mask = (ipToInt(str_inf_mask)&0xFFFFFFFF);
	var int_ip = (ipToInt(str_ip)&0xFFFFFFFF);
	if(int_ip == (int_inf_ip&int_inf_mask))
	{
	    return true;
	}
	return false;
}

function isBroadcastIp(str_ip,str_inf_ip,str_inf_mask)
{
	var int_inf_ip = (ipToInt(str_inf_ip)&0xFFFFFFFF);
	var int_inf_mask = (ipToInt(str_inf_mask)&0xFFFFFFFF);
	var int_ip = (ipToInt(str_ip)&0xFFFFFFFF);
	if((int_ip&int_inf_mask) == (int_inf_ip&int_inf_mask))
	{
	    if(((int_ip&(~int_inf_mask))==(~int_inf_mask)))
	    {
	        return true;
	    }
	}
	return false;
}

function getNetIp(str_ip,str_mask)
{
    if(!str_ip)
    {
        return 0x00000000;
    }
    var int_ip = ipToInt(str_ip);
    var int_mask = ipToInt(str_mask);
    return (int_ip&int_mask); 
}


function ipVerify(str_ip)

{
    var pattern_ip = /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])(\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])){3}$/;

    if(0 == str_ip.length)

	{


		return false;

	}

	if(!pattern_ip.test(str_ip))

	{


		return false;

	}

	var sub_array = str_ip.split('.');

    for(i = 0; i < 4; i++ )

    {

		if (sub_array[i] < 0 || sub_array[i] > 255)

		{


			return false;

		}

    }

	if(sub_array[0]==0)

	{


		return false;

	}

	if(sub_array[0]==255)

	{


		return false;

	}



    if(sub_array[0] >= 240 && sub_array[0] <=254)

	{


		return false;

	}


    if(sub_array[0] >= 224 && sub_array[0] <=239)

	{


		return false;

	}

    if(sub_array[0]==127)

	{

		return false;

	}

    return true;

}

function maskVerify(str_mask)

{
    var pattern_mask = /^\s*[0-9]{1,3}\.{1}[0-9]{1,3}\.{1}[0-9]{1,3}\.{1}[0-9]{1,3}\s*$/;
    if(0 == str_mask.length)
	{
		return false;
	}
    if(!pattern_mask.test(str_mask))

	{
		return false;

	}

	if(str_mask == "255.255.255.255" || str_mask == "0.0.0.0" || str_mask == "255.255.255.254")
	{
		return false;
	}

    var sub_array = str_mask.split('.');

    for(i = 0; i < 4; i++)

    {

        if (sub_array[i] < 0 || sub_array[i] > 255)

		{

			return false;

		}

    }



    var find_zero = false;

    var int_ip = parseInt(parseInt(parseInt(sub_array[0])<<24) + parseInt(parseInt(sub_array[1])<<16) + parseInt(parseInt(sub_array[2])<<8) + parseInt(sub_array[3]));


    for(i= 0; i < 32; ++i)

    {

        var flag = 1<<(31-i);

        if(parseInt(int_ip & flag) == parseInt(0))

        {

            find_zero = true;

        }

        else

        {

            if(find_zero == true)

			{

				return false;

			}

        }

    }

    return true;

}

