---
layout: post
title: Recent Blog Posts
---

<div class="home">
    {%- if site.posts.size > 0 -%}
        {%- for post in site.posts limit: 5 -%}
            <h3>
                <a class="post-link" href="{{ post.url | relative_url }}">
                {{ post.title | escape }}
                </a>
            </h3>
            <div id="page-preview" style="margin-left: 5%">
                <p>{{ post.excerpt }}</p>
            </div>
            <hr>
        {%- endfor -%}
    {%- endif -%}
    <p><h3><a href="posts.html">Other Posts...</a></h3></p>
</div>
