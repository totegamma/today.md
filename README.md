# today.md: Daily report based todo management application

## Features
### Main feature
This application manages your daily report. Just type "today" to open the daily report with your favorite editor(vim), and this application will rotate a file when the latest daily report doesn't today's one.

### Template system
You can create a template. And you can set the 'inherit section', like 'long-term-todo' or 'Do Next' section.

If you create a template like this,
```markdown
# Project
{{Project}}
# Todo
{{Do Next}}
# Did

# Do Next

```

And your today's report is like this,
```markdown
# Project
- [x] find a good restaurant
- [ ] decide when to go
- [ ] reserve a restaurant
# Todo
Hear some noice restaurants form co-workers.
# Did
I found 3 good shops today.
- a-shop
- b-shop
- c-shop
# Do Next
Schedule adjustment with friends.
```

You can get this page tomorrow.
```markdown
# Project
- [x] find a good restaurant
- [ ] decide when to go
- [ ] reserve a restaurant
# Todo
Schedule adjustment with friends.
# Did

# Do Next
```

Template file should be named ".template.md' and placed in the root of the project folder.

### git hooks
Managing todos with git would be nice!
This application will create a sub-report in the pre-commit hook.

### Quick memo
`today memo` creates a new file in the today's directory and opens it.
You can specify the memo name (the default value is timestamp).

It would be nice to create alias `alias memo today memo`

## Install & QuickStart
`git clone --recursive` then `make` then `sudo make install`
You may need to adjust the g++ version in the makefile. 

After tool instration, you have to create at least 1 todo project with
```
today init <project path>
```
this commands works like `git init <path>`
and also automatically adds the path to .today.conf

Then, you can use 'today' command to open your today's report!

## Configuration
You can configure this application under your home directory and the name `.today.conf`
Most of all configurations are override-able with commandline options.

.today.conf is like this
```toml
# editor
editor="vim"

# project ID (current pointer value that points project list)
id=0

# project list
projects=["/home/myname/my1stProject", "/home/myname/my2ndProject"]

# hidden option (internal use)
configured=true

# sections to reflect (add section names to use in template)
reflect.sections="DoNext"
```
