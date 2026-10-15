This update fixes collection title selection and scrolling in Mobile Showcase. It also corrects Decades hit areas beneath sticky headings and fits Mobile collection layouts to the available width, including rotation and larger UI sizes.

It updates **Nuvio TV/Mobile Test** in place with the same package and signing certificate. Saved accounts and profiles remain in the existing app.

Use the numbered checks in the attached test instructions. PR publication remains on hold until device testing is complete.

1. Install **Nuvio TV/Mobile Test — Logs11** using **Update**. Open the same profile and confirm your saved accounts are still present.
2. Open **Settings → Home screen → Collections → Default collection view**, choose **Showcase**, then open a populated collection. Tap a title other than the first; its own title page should open.
3. Return to the collection. Swipe a Showcase row sideways, release and wait one second. It should stay where you left it without opening a title. Tap a newly revealed title; that title should open.
4. Swipe vertically to another Showcase row. The page should stay in place after release, and tapping that row should open the title you chose. The fixed **Display** control should remain usable.
5. From **Display**, try **Grid**, **Timeline**, **Ranking**, **Saga**, **Decades** and **Mural** in portrait. Scroll through a populated collection and open titles near the edges and later rows. With a larger UI size, every title should remain reachable. In **Decades**, tapping the sticky decade heading must not open a title hidden underneath it.
6. Rotate the phone while a collection remains open. The cards should fit the new width and remain selectable. Leave/reopen the collection or switch profiles; an old touch must not open a title on the new screen. Your saved collection view should remain selected.
