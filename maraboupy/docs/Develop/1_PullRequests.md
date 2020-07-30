# Pull Requests

1. All changes to be merged should reside on a branch of *your* fork.
    * Travis workers on the main Marabou repository are shared by everyone and
      should primarily be used for testing pull requests and `master`. You get
      separate Travis workers for testing branches on your fork, so it is highly
      encouraged that you use this for your own and the team's benefit.
    * If you made major changes, break down your changes into small chunks and
      submit a pull request for each chunk in order to make reviewing less
      painful.
    * It is recommended to not commit work in progress. You should always create
      pull requests for self-contained finished chunks. New features that are
      too big for one pull request should be split into smaller (self-contained)
      pull requests.
2. Create a pull request via GitHub and select at least one reviewer.
    * one reviewer for minor changes
    * two or more reviewers for everything that is not a minor change
3. You need approval for your pull request from one (minor changes) or two (more
   than minor changes) reviewers before you can merge your pull request.
4. Commits pushed to the pull request should only contain fixes and changes
   related to reviewer comments.
    * We require that pull requests are up-to-date with `master` before they can
      be merged. You can either update your branch on the GitHub page of your
      pull request or you can (while you are on the branch of your pull request)
      merge the current `master` using `git pull origin master` (this creates a
      merge commit that can be squashed when merging the pull request) and then
      `git push` to update the pull request.
5. Merging pull requests to master. This step can be done on the GitHub page of
   your pull request.
    * Merging is only possible after Travis has succeeded.
    * Authors of a pull request are expected to perform the merge if they have
      write access to the repository (reviewers should not merge pull requests
      of developers that have privileges to do that themselves).
    * Creating merge commits for pull requests is discouraged (we prefer a
      linear history) and therefore disabled via the GitHub interface.
    * Squash commits are preferred over rebase commits.
        - if the intermediate commits are not of general interest, do a squash commit
    * In most cases, Github should automatically add the pull request number
      (`#<number>`) to the title of the commit. Do not remove the number and add
      it if it is missing (useful for debugging/documentation purposes).