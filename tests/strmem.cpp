//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/stdout.hpp"
#include "../src/memory/memory.hpp"
#include "../src/std.hpp"

#include <cstring>

int
main(void)
{
  const char *test
      = " Sed ut perspiciatis, unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem "
        "aperiam eaque ipsa, quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt, explicabo. "
        "Nemo enim ipsam voluptatem, quia voluptas sit, aspernatur aut odit aut fugit, sed quia consequuntur magni "
        "dolores eos, qui ratione voluptatem sequi nesciunt, neque porro quisquam est, qui dolorem ipsum, quia dolor "
        "sit amet consectetur adipisci[ng] velit, sed quia non numquam [do] eius modi tempora inci[di]dunt, ut labore "
        "et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum[d] exercitationem ullam "
        "corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? [D]Quis autem vel eum i[r]ure "
        "reprehenderit, qui in ea voluptate velit esse, quam nihil molestiae consequatur, vel illum, qui dolorem eum "
        "fugiat, quo voluptas nulla pariatur? At vero eos et accusamus et iusto odio dignissimos ducimus, qui "
        "blanditiis praesentium voluptatum deleniti atque corrupti, quos dolores et quas molestias excepturi sint, "
        "obcaecati cupiditate non provident, similique sunt in culpa, qui officia deserunt mollitia animi, id est "
        "laborum et dolorum fuga. Et harum quidem reru[d]um facilis est e[r]t expedita distinctio. Nam libero tempore, "
        "cum soluta nobis est eligendi optio, cumque nihil impedit, quo minus id, quod maxime placeat facere possimus, "
        "omnis voluptas assumenda est, omnis dolor repellend[a]us. Temporibus autem quibusdam et aut officiis debitis "
        "aut rerum necessitatibus saepe eveniet, ut et voluptates repudiandae sint et molestiae non recusandae. Itaque "
        "earum rerum hic tenetur a sapiente delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut "
        "perferendis doloribus asperiores repellat.";
  mc::io::stdoutln(mc::strstr(test, "omnis"));
  volatile int x = 0;
  size_t cnt = 0;
  for ( uintmax_t i = 0; i < 1e9; i++ ) {
    cnt = micron::strlen(test);
    x++;
  }
  mc::io::stdoutln(cnt);
}
