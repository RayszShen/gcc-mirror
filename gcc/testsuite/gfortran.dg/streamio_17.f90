! { dg-do run }
program stream_test
implicit none
integer :: ios
character(128) :: message
open(10, status='scratch', access='stream')
write (10, rec=1, iostat=ios, iomsg=message) "This is a test" !
if (ios.ne.5001) call abort
if (message.ne. &
  &"Record number not allowed for stream access data transfer") &
  call abort
end program
