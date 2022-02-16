from netcat import Netcat

sender_mail = input('Enter sender\'s email :')

receiver_mail = input('Enter receiver\'s email :')

subject = input('Enter subject :')

filename = input('Enter filename :')

nc_ins = Netcat()

f=open(filename,'r')

nc_ins.connect('mail-relay.iu.edu','25')

raw_data = nc_ins.read(1024)

nc_ins.write('HELO 10.0.0.35\r\n')
raw_data = nc_ins.read(1024)

nc_ins.write('MAIL FROM:'+sender_mail+'\r\n')
raw_data = nc_ins.read(1024)

nc_ins.write('RCPT TO:'+receiver_mail+'\r\n')
raw_data = nc_ins.read(1024)

nc_ins.write('DATA\r\n')
raw_data = nc_ins.read(1024)

nc_ins.write('Subject:'+subject+'\r\n')
read_stuff = f.read()
nc_ins.write(''+read_stuff+'\r\n')

nc_ins.write('.\r\n')
raw_data = nc_ins.read(1024)

nc_ins.write('QUIT\r\n')
raw_data = nc_ins.read(1024)

nc_ins.close()
